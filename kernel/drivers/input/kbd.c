/* kbd.c - keyboard input.
 *
 * The Plus ROM's keyboard driver runs from the VIA interrupt:
 * it polls the keyboard, keeps the KeyMap up to date and asks a key
 * translation procedure (low-memory Key1Trans/Key2Trans) for a character.
 * The ROM leaves those procedures empty for the System file to fill in;
 * PlusOS installs plusos_key_trans (keytrans.S -> kbd_translate below).
 *
 * kbd_translate puts every key-down straight into our own ring buffer and
 * returns 0 to the ROM. With 0 the ROM posts no event and simply asks the
 * keyboard for the next key - the same path the ROM takes with no System
 * file at all, so the Event Manager is not involved in typing. Auto-repeat
 * is done here from the KeyMap.
 *
 * Fallback: if the ROM never calls the translator, poll the KeyMap.
 *
 * Diagnostics: kbd_status() describes the driver state; the console shows it
 * on the bottom line (see con_status).
 */
#include "kernel.h"
#include "proc.h"

static uint16_t mods;

#define HISTORY 20
static char history[HISTORY][80];
static int hist_count;
static int hist_loaded;
static const unsigned char hist_file[] = "\x0E" "PlusOS History";
static char hist_iobuf[HISTORY * 82];
static uint32_t status_hold_until;

/* ---- state shared with the keyboard interrupt ------------------------------ */
#define RING 32
static volatile uint8_t  ring[RING];
static volatile uint8_t  ring_head, ring_tail;
static volatile uint32_t trans_calls;        /* times the ROM called the translator */
static volatile uint8_t  last_code = 0xFF;   /* last raw key code (keypad = +$40) */
static volatile uint8_t  last_down;
static volatile uint16_t last_mods;
static volatile int16_t  rep_code = -1;      /* key held for auto-repeat */
static volatile uint8_t  rep_char;
static volatile uint32_t rep_time;
static uint8_t repeating;

/* raw transition log for the `keys` diagnostic: code | 0x80 if key down */
static volatile uint8_t rawlog[RING];
static volatile uint8_t raw_head, raw_tail;

int kbd_raw_next(void)
{
    if (raw_tail == raw_head) return -1;
    int v = rawlog[raw_tail];
    raw_tail = (uint8_t)((raw_tail + 1) % RING);
    return v;
}

static uint8_t *keymap(void) { return (uint8_t *)LM_ADDR_KeyMap; }
static int key_is_down(int code) { return (keymap()[code >> 3] >> (code & 7)) & 1; }

static int translate(int code, int modword) { return keymap_translate(code, modword); }

static void ring_put(uint8_t c)
{
    uint8_t next = (uint8_t)((ring_head + 1) % RING);
    if (next != ring_tail) { ring[ring_head] = c; ring_head = next; }
}

/* Called by the ROM keyboard driver (interrupt level) through keytrans.S. */
int kbd_translate(int code, int modword)
{
    trans_calls++;
    last_code = (uint8_t)code;
    last_mods = (uint16_t)modword;
    int down = key_is_down(code);           /* the ROM updated the KeyMap just before */
    last_down = (uint8_t)down;
    uint8_t rn = (uint8_t)((raw_head + 1) % RING);
    if (rn != raw_tail) { rawlog[raw_head] = (uint8_t)(code | (down ? 0x80 : 0)); raw_head = rn; }
    int c = translate(code, modword);
    if (down) {
        if (c) {
            ring_put((uint8_t)c);
            rep_code = (int16_t)code;
            rep_char = (uint8_t)c;
            rep_time = LM_LONG(0x16A);
            repeating = 0;
        }
    } else if (code == rep_code) {
        rep_code = -1;
    }
    return 0;                               /* ROM: post nothing, poll for the next key */
}

/* ---- fallback: KeyMap polling --------------------------------------------- */
static uint8_t prev[16];

static int keymap_poll(void)
{
    uint8_t cur[16];
    memcpy(cur, keymap(), 16);
    uint16_t modword = (uint16_t)((cur[6] << 8) | cur[7]);
    for (int i = 0; i < 16; i++) {
        prev[i] &= cur[i];                   /* forget released keys */
        uint8_t fresh = cur[i] & ~prev[i];
        if (!fresh) continue;
        for (int b = 0; b < 8; b++) {
            if (!(fresh & (1 << b))) continue;
            prev[i] |= (uint8_t)(1 << b);    /* report each new key exactly once */
            int c = translate(i * 8 + b, modword);
            if (c) return c;
        }
    }
    return 0;
}

/* ---- public ------------------------------------------------------------------- */
extern void plusos_key_trans(void);

void kbd_init(int events)
{
    (void)events;
    memcpy(prev, keymap(), 16);
    if (g_hw.has_adb || !machine_is_plus()) /* ADB Macs (SE/Classic/II/LC/Quadra): driver updates KeyMap */
        return;
    uint16_t sr;
    __asm__ volatile("move.w %%sr,%0\n\tori.w #0x0700,%%sr" : "=d"(sr));      /* no interrupts */
    LM_PTR(LM_ADDR_Key1Trans) = (void *)plusos_key_trans;
    LM_PTR(LM_ADDR_Key2Trans) = (void *)plusos_key_trans;
    LM_BYTE(LM_ADDR_ScrDmpType) = 0xFF;     /* untranslated keys: no screen-dump event */
    __asm__ volatile("move.w %0,%%sr" : : "d"(sr));
}

int kbd_events(void) { return trans_calls != 0; }
uint16_t kbd_mods(void) { return last_mods; }

/* KeyMap path (Mac SE, or a Plus whose translator is never called): the key
 * that just went down, with auto-repeat while it stays down. */
static int keymap_key(void)
{
    uint8_t before[16];
    memcpy(before, prev, 16);
    int c = keymap_poll();
    if (c) {
        for (int i = 0; i < 16; i++) {      /* remember which key it was, for repeat */
            uint8_t fresh = prev[i] & ~before[i];
            for (int b = 0; fresh && b < 8; b++)
                if (fresh & (1 << b)) { rep_code = (int16_t)(i * 8 + b); fresh = 0; }
        }
        last_code = (uint8_t)rep_code;
        last_down = 1;
        rep_char = (uint8_t)c;
        rep_time = LM_LONG(0x16A);
        repeating = 0;
    }
    return c;
}

int kbd_poll(void)
{
    if (trans_calls == 0) {                 /* translator not in use: poll the KeyMap */
        int k = keymap_key();
        if (k) return trans_calls ? 0 : k;  /* Plus translator started meanwhile: it has the key */
        goto repeat;
    }
    if (ring_tail != ring_head) {
        int c = ring[ring_tail];
        ring_tail = (uint8_t)((ring_tail + 1) % RING);
        mods = last_mods;
        return c;
    }
repeat:
    /* auto-repeat: first after 1/2 s, then 15 per second */
    ;
    int16_t rc = rep_code;
    if (rc >= 0) {
        if (!key_is_down(rc)) { rep_code = -1; return 0; }
        uint32_t now = LM_LONG(0x16A);
        if (now - rep_time >= (repeating ? 4u : 30u)) {
            rep_time = now;
            repeating = 1;
            return rep_char;
        }
    }
    return 0;
}

int kbd_status(char *buf, int size)
{
    return snprintf(buf, size, " KBD type %02x  trans %lu  last %02x %s  mods %04x  buf %d  ticks %lu ",
                    LM_BYTE(LM_ADDR_KbdType), (unsigned long)trans_calls, last_code,
                    last_down ? "dn" : "up", last_mods,
                    (ring_head - ring_tail + RING) % RING, (unsigned long)LM_LONG(0x16A));
}

/* Block the current process until a key is available (used by SYS_waitkey and,
 * once the scheduler is running, by kbd_get). Sleeps a tick between polls so the
 * idle task can halt; works on both the Plus (interrupt-filled ring) and the SE
 * (polled KeyMap). */
int kbd_wait(void)
{
    extern volatile int sched_active;
    int c;
    while (!(c = kbd_poll())) {
        if (sched_active) proc_sleep_ticks(1);
        else con_blink();
    }
    return c;
}

int kbd_get(void)
{
    extern volatile int sched_active;
    int c;
    uint32_t shown = 0;
    con_set_cursor(1);
    while (!(c = kbd_poll())) {
        con_blink();
        uint32_t t = LM_LONG(0x16A);
        if (t >= status_hold_until && t - shown >= 10) { /* refresh the status line ~6x a second */
            shown = t;
            char s[80];
            kbd_status(s, sizeof s);
            con_status(s);
        }
        if (sched_active) proc_sleep_ticks(1);   /* let other tasks run while we wait */
    }
    con_set_cursor(0);
    return c;
}

/* Like kbd_get, but a mouse click also counts: returns KEY_CLICK, so a
 * keyboard with a dead Return key can't lock you out. */
int kbd_get_click(void)
{
    extern volatile int sched_active;
    int c;
    uint32_t shown = 0;
    while (Button()) {}                     /* don't take a click that is already down */
    con_set_cursor(1);
    for (;;) {
        if ((c = kbd_poll())) break;
        if (Button()) {
            while (Button()) {}
            c = KEY_CLICK;
            break;
        }
        con_blink();
        uint32_t t = LM_LONG(0x16A);
        if (t >= status_hold_until && t - shown >= 10) {
            shown = t;
            char s[80];
            kbd_status(s, sizeof s);
            con_status(s);
        }
        if (sched_active) proc_sleep_ticks(1);   /* yield to other tasks while waiting */
    }
    con_set_cursor(0);
    return c;
}

/* ---- Persistent 20-command history ("PlusOS History" on boot volume) ------- */
static void kbd_history_load(void)
{
    if (hist_loaded) return;
    if (!(g_flags & PLUSOS_HAVE_FS)) return;
    hist_loaded = 1;
    INTEGER ref;
    if (FSOpen(hist_file, g_boot_vref, &ref) != 0) return;
    LONGINT n = (LONGINT)sizeof(hist_iobuf) - 1;
    FSRead(ref, &n, hist_iobuf);
    FSClose(ref);
    if (n <= 0) return;
    hist_iobuf[n] = 0;
    char *p = hist_iobuf;
    while (*p) {
        while (*p == '\r' || *p == '\n') p++;
        if (!*p) break;
        char *line = p;
        while (*p && *p != '\r' && *p != '\n') p++;
        if (*p) *p++ = 0;
        if (line[0]) {
            strlcpy(history[hist_count % HISTORY], line, sizeof history[0]);
            hist_count++;
        }
    }
}

static void kbd_history_save(void)
{
    if (!(g_flags & PLUSOS_HAVE_FS)) return;
    int n = hist_count < HISTORY ? hist_count : HISTORY;
    int start = hist_count - n;
    int len = 0;
    for (int i = 0; i < n; i++) {
        const char *s = history[(start + i) % HISTORY];
        while (*s && len < (int)sizeof(hist_iobuf) - 2)
            hist_iobuf[len++] = *s++;
        hist_iobuf[len++] = '\n';
    }
    INTEGER ref;
    if (FSOpen(hist_file, g_boot_vref, &ref) != 0) {
        Create(hist_file, g_boot_vref, 0x504C4F53 /* 'PLOS' */, 0x54455854 /* 'TEXT' */);
        if (FSOpen(hist_file, g_boot_vref, &ref) != 0) return;
    }
    LONGINT wlen = len;
    SetFPos(ref, 1, 0);
    FSWrite(ref, &wlen, hist_iobuf);
    SetEOF(ref, len);
    FSClose(ref);
    FlushVol(0, g_boot_vref);
}

int kbd_history_count(void)
{
    kbd_history_load();
    return hist_count < HISTORY ? hist_count : HISTORY;
}

const char *kbd_history_get(int index)
{
    kbd_history_load();
    int n = hist_count < HISTORY ? hist_count : HISTORY;
    if (index < 0 || index >= n) return 0;
    return history[(hist_count - n + index) % HISTORY];
}

void kbd_history_add(const char *line)
{
    kbd_history_load();
    if (!line) return;
    while (*line == ' ' || *line == '\t') line++;
    if (!*line) return;
    if (hist_count > 0 && !strcmp(history[(hist_count - 1) % HISTORY], line))
        return;
    strlcpy(history[hist_count % HISTORY], line, sizeof history[0]);
    hist_count++;
    kbd_history_save();
}

void kbd_history_clear(void)
{
    hist_loaded = 1;
    hist_count = 0;
    memset(history, 0, sizeof history);
    kbd_history_save();
}

/* ---- Tab Auto-Completion (commands, :bin:, raw app store, and HFS files) --- */
#define MAX_CANDS 48
#define CAND_LEN  32

static void cand_add(char cands[MAX_CANDS][CAND_LEN], int *ncands,
                     const char *name, const char *prefix, int plen, int force_lower)
{
    if (*ncands >= MAX_CANDS || !name || !name[0]) return;
    if (plen > 0 && strncasecmp(name, prefix, (size_t)plen) != 0) return;
    char tmp[CAND_LEN];
    int i = 0;
    int keep_upper = (plen > 0 && prefix[0] >= 'A' && prefix[0] <= 'Z');
    for (; name[i] && i < CAND_LEN - 1; i++) {
        char c = name[i];
        if (force_lower && !keep_upper && c >= 'A' && c <= 'Z')
            c = (char)(c - 'A' + 'a');
        tmp[i] = c;
    }
    tmp[i] = 0;
    for (int j = 0; j < *ncands; j++)
        if (!strcasecmp(cands[j], tmp)) return;
    strlcpy(cands[(*ncands)++], tmp, CAND_LEN);
}

static long hfs_find_subdir(long parent_dir, const char *folder_name)
{
    if (!(g_flags & PLUSOS_HAVE_FS)) return 0;
    for (int idx = 1; idx < 128; idx++) {
        CInfoPBRec pb;
        Str255 nm;
        memset(&pb, 0, sizeof pb);
        pb.dirInfo.ioNamePtr = nm;
        pb.dirInfo.ioVRefNum = g_boot_vref;
        pb.dirInfo.ioDrDirID = parent_dir;
        pb.dirInfo.ioFDirIndex = (INTEGER)idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        char cname[64];
        p2cstr_buf(cname, nm);
        if ((pb.dirInfo.ioFlAttrib & 0x10) && !strcasecmp(cname, folder_name))
            return pb.dirInfo.ioDrDirID;
    }
    return 0;
}

static void hfs_collect_dir(long dir_id, char cands[MAX_CANDS][CAND_LEN], int *ncands,
                            const char *prefix, int plen, int force_lower, int append_colon)
{
    if (!(g_flags & PLUSOS_HAVE_FS) || !dir_id) return;
    for (int idx = 1; idx < 128 && *ncands < MAX_CANDS; idx++) {
        CInfoPBRec pb;
        Str255 nm;
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = nm;
        pb.hFileInfo.ioVRefNum = g_boot_vref;
        pb.hFileInfo.ioDirID = dir_id;
        pb.hFileInfo.ioFDirIndex = (INTEGER)idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        char cname[64];
        p2cstr_buf(cname, nm);
        if (append_colon && (pb.hFileInfo.ioFlAttrib & 0x10)) {
            size_t l = strlen(cname);
            if (l + 1 < sizeof cname) { cname[l] = ':'; cname[l + 1] = 0; }
        }
        cand_add(cands, ncands, cname, prefix, plen, force_lower);
    }
}

int kbd_tab_complete(const char *base_buf, int base_len, char *buf, int size,
                     int tab_idx, char *status_out, int status_sz)
{
    static const char * const builtins[] = {
        "help", "history", "ls", "dir", "cat", "type", "run", "ps", "uptime",
        "mem", "rom", "trap", "time", "date", "gui", "paint", "mouse", "keys",
        "beep", "peek", "poke", "echo", "cls", "clear", "ver", "menu", "boot",
        "drives", "reboot", "off", "shutdown", "exit", "jobs", "fg", "theme", 0
    };
    if (status_out && status_sz > 0) status_out[0] = 0;

    int word_start = base_len;
    while (word_start > 0 && base_buf[word_start - 1] != ' ' && base_buf[word_start - 1] != '\t')
        word_start--;
    int first_nonspace = 0;
    while (first_nonspace < word_start && (base_buf[first_nonspace] == ' ' || base_buf[first_nonspace] == '\t'))
        first_nonspace++;
    int is_first_word = (first_nonspace == word_start);

    const char *word = base_buf + word_start;
    int word_len = base_len - word_start;
    int last_colon = -1;
    for (int i = 0; i < word_len; i++)
        if (word[i] == ':') last_colon = i;

    char dir_prefix[40];
    const char *leaf = word;
    int leaf_len = word_len;
    if (last_colon >= 0) {
        int dlen = last_colon + 1;
        if (dlen >= (int)sizeof dir_prefix) dlen = (int)sizeof dir_prefix - 1;
        memcpy(dir_prefix, word, (size_t)dlen);
        dir_prefix[dlen] = 0;
        leaf = word + last_colon + 1;
        leaf_len = word_len - (last_colon + 1);
    } else {
        dir_prefix[0] = 0;
    }

    char cands[MAX_CANDS][CAND_LEN];
    int ncands = 0;

    if (last_colon >= 0) {
        /* Completing an HFS path such as :Desktop:Read or :Games:CUBE3D */
        long dir_id = 2; /* root */
        if (last_colon > 0) {
            char folder[36];
            int s = (word[0] == ':') ? 1 : 0;
            int flen = last_colon - s;
            if (flen > 0 && flen < (int)sizeof folder) {
                memcpy(folder, word + s, (size_t)flen);
                folder[flen] = 0;
                dir_id = hfs_find_subdir(2, folder);
            }
        }
        hfs_collect_dir(dir_id, cands, &ncands, leaf, leaf_len, 0, 1);
    } else if (is_first_word) {
        /* Completing a command name from built-ins, :bin:, or raw app store */
        for (int i = 0; builtins[i]; i++)
            cand_add(cands, &ncands, builtins[i], leaf, leaf_len, 1);
        long bin_id = hfs_find_subdir(2, "bin");
        if (bin_id)
            hfs_collect_dir(bin_id, cands, &ncands, leaf, leaf_len, 1, 0);
        uint8_t *rawdir = kmalloc(2048);
        if (rawdir) {
            if (blk_read(APPS_BLOCK, 4, rawdir) == 0 && !memcmp(rawdir, "PLDR", 4)) {
                uint32_t count = *(uint32_t *)(rawdir + 4);
                for (uint32_t i = 0; i < count && i < 85; i++) {
                    char rname[17];
                    memcpy(rname, rawdir + 8 + i * 24, 16);
                    rname[16] = 0;
                    cand_add(cands, &ncands, rname, leaf, leaf_len, 1);
                }
            }
            kfree(rawdir);
        }
    } else {
        /* Completing an argument: check root files/folders, :Desktop:, and :bin: */
        hfs_collect_dir(2, cands, &ncands, leaf, leaf_len, 0, 1);
        long desk_id = hfs_find_subdir(2, "Desktop");
        if (desk_id)
            hfs_collect_dir(desk_id, cands, &ncands, leaf, leaf_len, 0, 0);
        long bin_id = hfs_find_subdir(2, "bin");
        if (bin_id)
            hfs_collect_dir(bin_id, cands, &ncands, leaf, leaf_len, 1, 0);
    }

    /* Sort candidates alphabetically (case-insensitive) */
    for (int i = 0; i < ncands - 1; i++) {
        for (int j = i + 1; j < ncands; j++) {
            if (strcasecmp(cands[i], cands[j]) > 0) {
                char tmp[CAND_LEN];
                memcpy(tmp, cands[i], CAND_LEN);
                memcpy(cands[i], cands[j], CAND_LEN);
                memcpy(cands[j], tmp, CAND_LEN);
            }
        }
    }

    if (ncands == 0) {
        if (status_out && status_sz > 0)
            snprintf(status_out, (size_t)status_sz, " [Tab] no matches for '%.*s' ", word_len, word);
        return 0;
    }

    if (ncands == 1) {
        int pos = word_start;
        if (pos > size - 1) pos = size - 1;
        memcpy(buf, base_buf, (size_t)pos);
        buf[pos] = 0;
        size_t clen = strlen(cands[0]);
        const char *trail = (clen > 0 && cands[0][clen - 1] != ':') ? " " : "";
        snprintf(buf + pos, (size_t)(size - pos), "%s%s%s", dir_prefix, cands[0], trail);
        if (status_out && status_sz > 0)
            snprintf(status_out, (size_t)status_sz, " [Tab] completed: %s ", cands[0]);
        return 1;
    }

    /* Multiple matches: compute longest common prefix */
    int lcp = (int)strlen(cands[0]);
    for (int i = 1; i < ncands; i++) {
        int k = 0;
        while (k < lcp && tolower((unsigned char)cands[0][k]) == tolower((unsigned char)cands[i][k]))
            k++;
        lcp = k;
    }

    int pick = -1;
    if (tab_idx == 0 && lcp > leaf_len) {
        /* First Tab extends to the longest common prefix */
        int pos = word_start;
        if (pos > size - 1) pos = size - 1;
        memcpy(buf, base_buf, (size_t)pos);
        buf[pos] = 0;
        snprintf(buf + pos, (size_t)(size - pos), "%s%.*s", dir_prefix, lcp, cands[0]);
    } else {
        /* Subsequent Tabs (or when already at LCP) cycle through candidates */
        pick = (lcp > leaf_len) ? (tab_idx - 1) % ncands : tab_idx % ncands;
        int pos = word_start;
        if (pos > size - 1) pos = size - 1;
        memcpy(buf, base_buf, (size_t)pos);
        buf[pos] = 0;
        snprintf(buf + pos, (size_t)(size - pos), "%s%s", dir_prefix, cands[pick]);
    }

    if (status_out && status_sz > 0) {
        int p = snprintf(status_out, (size_t)status_sz, " [Tab %d] ", ncands);
        int start_i = (pick >= 6) ? (pick - 3) : 0;
        for (int i = start_i; i < ncands && p < status_sz - 4; i++) {
            int n = snprintf(status_out + p, (size_t)(status_sz - p),
                             (i == pick) ? "[%s] " : "%s ", cands[i]);
            if (n <= 0) break;
            p += n;
        }
    }
    return 2;
}

/* Line editor: printable keys insert, Backspace deletes, Up/Down walk the
 * persistent 20-command history, Tab auto-completes, Return / Enter / click
 * finishes. Returns the length. */
int kbd_line(char *buf, int size)
{
    kbd_history_load();
    int len = 0, hpos = hist_count;
    int tab_state = 0, tab_base_len = 0;
    char tab_base[128];
    buf[0] = 0;
    for (;;) {
        int c = kbd_get_click();
        if (c != KEY_TAB) tab_state = 0;
        if (c == KEY_RETURN || c == KEY_ENTER || c == KEY_CLICK) {
            status_hold_until = 0;
            con_putc('\n');
            break;
        } else if (c == KEY_BACKSPACE || c == 0x7F) {
            if (len) { len--; buf[len] = 0; con_putc('\b'); }
        } else if (c == 0x15 || c == KEY_ESC) {         /* Ctrl+U or Esc clears input line */
            while (len) { len--; con_putc('\b'); }
            buf[0] = 0;
        } else if (c == KEY_TAB) {
            if (tab_state == 0) {
                buf[len] = 0;
                strlcpy(tab_base, buf, sizeof tab_base);
                tab_base_len = len;
            }
            char st[80];
            int rc = kbd_tab_complete(tab_base, tab_base_len, buf, size, tab_state, st, sizeof st);
            if (rc > 0) {
                while (len) { len--; con_putc('\b'); }
                len = (int)strlen(buf);
                con_write(buf, (size_t)len);
                if (rc == 1) tab_state = 0;             /* unique completion finished */
                else tab_state++;
            }
            if (st[0]) {
                status_hold_until = LM_LONG(0x16A) + 180;
                con_status(st);
            }
        } else if (c == KEY_UP || c == KEY_DOWN) {
            int n = hist_count < HISTORY ? hist_count : HISTORY;
            if (!n) continue;
            if (c == KEY_UP && hpos > hist_count - n) hpos--;
            else if (c == KEY_DOWN && hpos < hist_count) hpos++;
            while (len) { len--; con_putc('\b'); }
            buf[0] = 0;
            if (hpos < hist_count) {
                strlcpy(buf, history[hpos % HISTORY], (size_t)size);
                len = (int)strlen(buf);
                con_write(buf, (size_t)len);
            }
        } else if (c >= 0x20 && c < 0x7F && len < size - 1) {
            buf[len++] = (char)c;
            buf[len] = 0;
            con_putc(c);
        }
    }
    buf[len] = 0;
    if (len) kbd_history_add(buf);
    return len;
}

/* libc back-end for the kernel */
int __plusos_key_get(void)                 { return kbd_get(); }
int __plusos_key_line(char *buf, int size) { return kbd_line(buf, size); }
