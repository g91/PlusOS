/* hexedit - view and edit any file byte by byte.
 *
 *   hexedit [file]
 *
 * Arrows / PgUp / PgDn / Home / End move, Tab switches between the hex and the
 * text column, typing 0-9 a-f (hex) or any character (text) changes the byte
 * under the cursor; typing past the end appends. Esc opens the command line:
 * S save, W save and quit, Q quit, G go to an offset.
 * Only the rows that change are redrawn, in place, so nothing flickers.
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusrom/FileMgr.h>

#define ROW_BYTES 8
static int PAGE_ROWS = 36;      /* set from the console size at start */
#define FIRST_ROW 2
static int HELP_ROW = 39;
static int MSG_ROW = 40;
static int LINE_W = 63;          /* columns - 1, so lines never wrap */
#define MAX_SIZE  60000L
#define GROW      4096L

static unsigned char *buf;
static long size, cap, cur, top;
static int  text_mode, nib, dirty;
static char path[80];
static INTEGER vref;

static void c2p(unsigned char *p, const char *c)
{
    int n = 0; while (c[n] && n < 254) { p[n + 1] = (unsigned char)c[n]; n++; } p[0] = (unsigned char)n;
}
static int has_colon(const char *s) { for (; *s; s++) if (*s == ':') return 1; return 0; }

static void clear_line(int row) { con_goto(0, row); printf("%-*s", LINE_W, ""); con_goto(0, row); }

static void header(void)
{
    con_goto(0, 0);
    con_invert(1);
    char h[80];
    snprintf(h, sizeof h, " HEXEDIT  %s   %ld bytes %s", path, size, dirty ? " *modified*" : "");
    printf("%-*s", LINE_W, h);
    con_invert(0);
}

static void draw_row(int r)
{
    long base = top + (long)r * ROW_BYTES;
    con_goto(0, FIRST_ROW + r);
    if (base > size || (base == size && !(cur == size && cur >= base && cur < base + ROW_BYTES))) {
        printf("%-*s", LINE_W, "");
        return;
    }
    printf("%06lx  ", base);
    for (int i = 0; i < ROW_BYTES; i++) {
        long o = base + i;
        if (o < size || o == cur) {
            int hl = o == cur && !text_mode;
            if (hl) con_invert(1);
            if (o < size) printf("%02x", buf[o]); else printf("__");
            if (hl) con_invert(0);
            printf(" ");
        } else printf("   ");
    }
    printf(" ");
    for (int i = 0; i < ROW_BYTES; i++) {
        long o = base + i;
        if (o < size || o == cur) {
            int c = o < size ? buf[o] : '_';
            if (c < 0x20 || c > 0x7E) c = '.';
            int hl = o == cur && text_mode;
            if (hl) con_invert(1);
            plusos->con_putc(c);
            if (hl) con_invert(0);
        } else plusos->con_putc(' ');
    }
    if (LINE_W > 41) printf("%-*s", LINE_W - 41, "");
}

static void draw_all(void)
{
    header();
    for (int r = 0; r < PAGE_ROWS; r++) draw_row(r);
    con_goto(0, HELP_ROW);
    printf("%-*s", LINE_W, "Arrows PgUp PgDn move  Tab hex/text  Esc commands");
}

static int row_of(long o) { return (int)((o - top) / ROW_BYTES); }

static void move_to(long n)
{
    if (n < 0) n = 0;
    if (n > size) n = size;                         /* one past the end = append */
    long old = cur;
    cur = n; nib = 0;
    long page = (long)PAGE_ROWS * ROW_BYTES;
    if (cur < top || cur >= top + page) {
        top = (cur / ROW_BYTES) * ROW_BYTES - (page / 2 / ROW_BYTES) * ROW_BYTES;
        if (top < 0) top = 0;
        for (int r = 0; r < PAGE_ROWS; r++) draw_row(r);
        return;
    }
    if (old >= top && old < top + page) draw_row(row_of(old));
    draw_row(row_of(cur));
}

static int set_byte(int v)
{
    if (cur == size) {                               /* append */
        if (size >= cap) {
            if (cap + GROW > MAX_SIZE) return 0;
            unsigned char *nb = malloc(cap + GROW);
            if (!nb) return 0;
            memcpy(nb, buf, size);
            free(buf); buf = nb; cap += GROW;
        }
        buf[size++] = 0;
    }
    buf[cur] = (unsigned char)v;
    if (!dirty) { dirty = 1; header(); }
    else if (cur == size - 1) header();
    return 1;
}

static int load(void)
{
    unsigned char p[256]; INTEGER ref;
    c2p(p, path);
    if (FSOpen(p, vref, &ref) != 0) return -1;
    LONGINT eof = 0;
    GetEOF(ref, &eof);
    if (eof > MAX_SIZE) { FSClose(ref); return -2; }
    size = eof;
    cap = size + GROW;
    buf = malloc(cap);
    if (!buf) { FSClose(ref); return -3; }
    LONGINT n = size;
    if (n) FSRead(ref, &n, buf);
    FSClose(ref);
    return 0;
}

static int save(void)
{
    unsigned char p[256]; INTEGER ref;
    c2p(p, path);
    if (FSOpen(p, vref, &ref) != 0) return -1;
    LONGINT n = size;
    int rc = (SetFPos(ref, 1, 0) == 0 && (n == 0 || FSWrite(ref, &n, buf) == 0)) ? 0 : -1;
    SetEOF(ref, size);
    FSClose(ref);
    FlushVol(0, vref);
    if (rc == 0) { dirty = 0; header(); }
    return rc;
}

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int resolve(const char *name)
{
    unsigned char p[256]; FInfo fi;
    const char *tries[3] = { ":Desktop:", "", ":bin:" };
    if (has_colon(name)) { snprintf(path, sizeof path, "%s", name); return 0; }
    for (int i = 0; i < 3; i++) {
        snprintf(path, sizeof path, "%s%s", tries[i], name);
        c2p(p, path);
        if (GetFInfo(p, vref, &fi) == 0) return 0;
    }
    return -1;
}

/* Esc menu; returns 1 to quit */
static int command(void)
{
    clear_line(MSG_ROW);
    printf("S save   W save+quit   Q quit   G go to offset   (other: back)");
    int k = plusos->key_get();
    clear_line(MSG_ROW);
    if (k == 's' || k == 'S' || k == 'w' || k == 'W') {
        if (save() != 0) { printf("Could not save the file."); return 0; }
        printf("Saved %ld bytes.", size);
        return k == 'w' || k == 'W';
    }
    if (k == 'q' || k == 'Q') {
        if (!dirty) return 1;
        printf("Discard your changes? (y/n) ");
        int a = plusos->key_get();
        clear_line(MSG_ROW);
        return a == 'y' || a == 'Y';
    }
    if (k == 'g' || k == 'G') {
        char line[16];
        printf("Go to offset (hex): ");
        plusos->key_line(line, sizeof line);
        long v = 0;
        for (char *s = line; *s; s++) { int h = hexval(*s); if (h < 0) break; v = v * 16 + h; }
        clear_line(MSG_ROW);
        move_to(v);
    }
    return 0;
}

int main(int argc, char **argv)
{
    vref = (INTEGER)plusos->boot_vrefnum;
    {   /* fit the layout to this console: the full screen, or a terminal window */
        int cols = 64, rows = 41;
        struct plusos_con_args c = { PLUSOS_CON_GETSIZE, 0, 0, 0, 0, &cols, &rows };
        plusos_syscall(SYS_console, &c);
        LINE_W = cols - 1;
        PAGE_ROWS = rows - 5 > 4 ? rows - 5 : 4;
        HELP_ROW = rows - 2;
        MSG_ROW = rows - 1;
    }
    plusos->con_cursor(0);
    con_clear();
    char name[64];
    if (argc >= 2) snprintf(name, sizeof name, "%s", argv[1]);
    else {
        printf("Hex editor - file to open: ");
        plusos->key_line(name, sizeof name);
        if (!name[0]) return 0;
    }
    if (resolve(name) != 0) { printf("\n%s: not found\n", name); return 1; }
    int rc = load();
    if (rc == -2) { printf("\n%s is larger than %ld bytes.\n", path, MAX_SIZE); return 1; }
    if (rc != 0) { printf("\nCannot open %s.\n", path); return 1; }

    con_clear();
    draw_all();
    for (;;) {
        int k = plusos->key_get();
        if (k == KEY_LEFT || k == KEY_BACKSPACE) move_to(cur - 1);
        else if (k == KEY_RIGHT) move_to(cur + 1);
        else if (k == KEY_UP) move_to(cur - ROW_BYTES);
        else if (k == KEY_DOWN) move_to(cur + ROW_BYTES);
        else if (k == KEY_PGUP) move_to(cur - (long)PAGE_ROWS * ROW_BYTES);
        else if (k == KEY_PGDN) move_to(cur + (long)PAGE_ROWS * ROW_BYTES);
        else if (k == KEY_HOME) move_to(0);
        else if (k == KEY_END) move_to(size);
        else if (k == KEY_TAB) { text_mode = !text_mode; nib = 0; draw_row(row_of(cur)); }
        else if (k == KEY_ESC) { if (command()) break; }
        else if (text_mode && k >= 0x20 && k < 0x7F) {
            if (set_byte(k)) move_to(cur + 1);
        } else if (!text_mode && hexval(k) >= 0) {
            int h = hexval(k);
            int v = cur < size ? buf[cur] : 0;
            v = nib ? ((v & 0xF0) | h) : ((h << 4) | (v & 0x0F));
            if (set_byte(v)) {
                if (nib) move_to(cur + 1);
                else { nib = 1; draw_row(row_of(cur)); }
            }
        }
    }
    con_clear();
    return 0;
}
