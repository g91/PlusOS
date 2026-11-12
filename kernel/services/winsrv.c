/* winsrv.c - the PlusOS window server (kernel side).
 *
 * Programs own windows. Each window is a 1-bit bitmap the owner draws into
 * (PlusGFX), an event queue, and a title. The desktop is the compositor: it
 * lists the windows, puts them on screen, and posts mouse and key events to
 * the window under focus. Because every program is its own preemptible
 * process, any number of windows update at once while the desktop keeps
 * moving them around.
 *
 * Terminal windows: a program started with SYS_termspawn gets a text console
 * that is a window. It is created the first time the program prints or reads
 * a key, so graphical programs never get one. printf, key input and the line
 * editor of every process attached to it go there. When the program ends,
 * the window stays (marked finished) until the user closes it; closing a
 * live terminal ends the programs attached to it.
 */
#include "kernel.h"
#include "proc.h"
#include <plusos/syscall.h>

#define MAX_KWIN 12
#define EVQ      32
#define TERM_COLS 48
#define TERM_ROWS 18

typedef struct {
    int used, id, owner, term, dead;
    int x, y, w, h;
    char title[32];
    uint8_t *bits; int rowbytes;
    volatile uint32_t seq;
    struct plusos_event q[EVQ]; int qh, qt;
    int col, row, cols, rows, cursor_on, cursor_shown;   /* terminal */
    uint8_t inv;
} KWin;

static KWin kw[MAX_KWIN];
static int  next_id = 1;
static volatile uint32_t ws_seq = 1;

extern const uint8_t plusos_font8x8[95 * 8];

static KWin *win_get(int id)
{
    if (id <= 0) return 0;
    for (int i = 0; i < MAX_KWIN; i++) if (kw[i].used && kw[i].id == id) return &kw[i];
    return 0;
}

static void touch(KWin *k) { k->seq++; ws_seq++; }

static KWin *win_create(int w, int h, const char *title, int x, int y, int owner, int term)
{
    int maxw = video_width() - 16;
    int maxh = video_rows() - 42;
    if (maxw < 496) maxw = 496;
    if (maxh < 300) maxh = 300;
    if (w < 16) w = 16;
    if (h < 8) h = 8;
    if (w > maxw) w = maxw;
    if (h > maxh) h = maxh;
    KWin *k = 0;
    for (int i = 0; i < MAX_KWIN; i++) if (!kw[i].used) { k = &kw[i]; break; }
    if (!k) return 0;
    int rb = ((w + 15) >> 4) << 1;
    uint8_t *b = kmalloc((uint32_t)rb * (uint32_t)h);
    if (!b) return 0;
    memset(k, 0, sizeof *k);
    memset(b, 0, (uint32_t)rb * (uint32_t)h);
    k->used = 1; k->id = next_id++;
    k->owner = owner; k->term = term;
    k->x = x; k->y = y; k->w = w; k->h = h;
    k->bits = b; k->rowbytes = rb;
    strlcpy(k->title, title ? title : "Window", sizeof k->title);
    touch(k);
    return k;
}

static void win_destroy(KWin *k)
{
    int id = k->id;
    /* processes whose console is this window lose it (and are ended) */
    for (int i = 0; i < 16; i++) {
        Proc *p = proc_table_entry(i);
        if (p->state != PROC_UNUSED && p->con_win == id) {
            p->con_win = -1;
            if (p->ring == RING_USER) proc_kill(p->pid);
        }
    }
    kfree(k->bits);
    k->used = 0;
    ws_seq++;
    proc_wakeup(k);                             /* anyone waiting for an event */
}

static void post(KWin *k, const struct plusos_event *ev)
{
    int nt = (k->qt + 1) % EVQ;
    if (nt == k->qh) return;                    /* full: drop */
    k->q[k->qt] = *ev;
    k->qt = nt;
    proc_wakeup(k);
}

/* 1 = got an event, 0 = none (no wait), -1 = the window is gone */
static int get_event(int id, struct plusos_event *ev, int wait)
{
    for (;;) {
        uint16_t sr = int_disable();
        KWin *k = win_get(id);
        if (!k) { int_restore(sr); return -1; }
        if (k->qh != k->qt) {
            *ev = k->q[k->qh];
            k->qh = (k->qh + 1) % EVQ;
            int_restore(sr);
            return 1;
        }
        int_restore(sr);
        if (!wait) return 0;
        proc_sleep(k);
    }
}

/* A process is ending: windows it owns go away, except terminals, which
 * stay up (marked finished) so their last output can still be read. */
void winsrv_proc_exit(int pid)
{
    for (int i = 0; i < MAX_KWIN; i++) {
        KWin *k = &kw[i];
        if (!k->used || k->owner != pid) continue;
        if (k->term) {
            k->owner = 0; k->dead = 1;
            int n = (int)strlen(k->title);
            for (const char *s = " (ended)"; *s && n < (int)sizeof k->title - 1; s++) k->title[n++] = *s;
            k->title[n] = 0;
            touch(k);
        } else {
            win_destroy(k);
        }
    }
}

/* ======================================================================== */
/* terminal consoles                                                        */
/* ======================================================================== */
static void t_glyph(KWin *k, int c, int r, int ch, uint8_t inv)
{
    const uint8_t *g = plusos_font8x8 + ((ch >= 0x20 && ch < 0x7F) ? ch - 0x20 : '?' - 0x20) * 8;
    uint8_t *p = k->bits + r * 8 * k->rowbytes + c;
    for (int i = 0; i < 8; i++, p += k->rowbytes) *p = g[i] ^ inv;
}

static void t_cursor(KWin *k, int show)
{
    if (k->cursor_shown == show || k->col >= k->cols) return;
    uint8_t *p = k->bits + (k->row * 8 + 7) * k->rowbytes + k->col;
    *p ^= 0xFE;
    k->cursor_shown = show;
}

static void t_scroll(KWin *k)
{
    uint32_t line = (uint32_t)k->rowbytes * 8;
    memmove(k->bits, k->bits + line, line * (uint32_t)(k->rows - 1));
    memset(k->bits + line * (uint32_t)(k->rows - 1), 0, line);
}

static void t_putc(KWin *k, int c)
{
    switch (c) {
    case '\n': k->col = 0; if (++k->row >= k->rows) { k->row = k->rows - 1; t_scroll(k); } break;
    case '\r': k->col = 0; break;
    case '\b': if (k->col > 0) { k->col--; t_glyph(k, k->col, k->row, ' ', k->inv); } break;
    case '\t': do t_putc(k, ' '); while (k->col & 7); break;
    case '\f': memset(k->bits, 0, (uint32_t)k->rowbytes * (uint32_t)k->h); k->col = k->row = 0; break;
    default:
        if (k->col >= k->cols) { k->col = 0; if (++k->row >= k->rows) { k->row = k->rows - 1; t_scroll(k); } }
        t_glyph(k, k->col, k->row, c & 0xFF, k->inv);
        k->col++;
    }
}

static KWin *my_term(void)
{
    Proc *p = proc_current;
    if (p->con_win == -2) {                     /* first use: open the terminal now */
        KWin *k = win_create(TERM_COLS * 8, TERM_ROWS * 8, p->name, -1, -1, p->pid, 1);
        if (!k) { p->con_win = 0; return 0; }   /* no window: fall back to the screen */
        k->cols = TERM_COLS; k->rows = TERM_ROWS;
        p->con_win = k->id;
        return k;
    }
    return win_get(p->con_win);
}

/* Console operation for a process whose console is a window. Returns 1 if
 * handled here, 0 if the caller should use the full-screen console. */
int term_console_op(struct plusos_con_args *c, long *result)
{
    Proc *p = proc_current;
    if (p->con_win == 0) return 0;
    *result = 0;
    if (p->con_win == -1) return 1;             /* console gone: discard */
    KWin *k = my_term();
    if (!k) return p->con_win == 0 ? 0 : 1;
    int was = k->cursor_shown;
    t_cursor(k, 0);
    switch (c->op) {
    case PLUSOS_CON_PUTC:   t_putc(k, c->a); break;
    case PLUSOS_CON_WRITE:  for (int i = 0; i < c->n; i++) t_putc(k, (uint8_t)c->s[i]); break;
    case PLUSOS_CON_CLEAR:  t_putc(k, '\f'); break;
    case PLUSOS_CON_GOTO:
        k->col = c->a < 0 ? 0 : c->a >= k->cols ? k->cols - 1 : c->a;
        k->row = c->b < 0 ? 0 : c->b >= k->rows ? k->rows - 1 : c->b;
        break;
    case PLUSOS_CON_GETPOS: if (c->pa) *c->pa = k->col; if (c->pb) *c->pb = k->row; break;
    case PLUSOS_CON_INVERT: k->inv = c->a ? 0xFF : 0; break;
    case PLUSOS_CON_CURSOR: k->cursor_on = c->a; break;
    case PLUSOS_CON_GETSIZE: if (c->pa) *c->pa = k->cols; if (c->pb) *c->pb = k->rows; break;
    }
    if (was && k->cursor_on) t_cursor(k, 1);
    touch(k);
    return 1;
}

/* key input for a process whose console is a window: 1 if handled */
int term_getkey(int wait, int *key)
{
    Proc *p = proc_current;
    if (p->con_win == 0) return 0;
    if (p->con_win == -1) { *key = wait ? 0x1B : 0; return 1; }
    KWin *k = my_term();
    if (!k) return 0;
    for (;;) {
        if (wait && k->cursor_on) { t_cursor(k, 1); touch(k); }
        struct plusos_event ev;
        int r = get_event(k->id, &ev, wait);
        k = win_get(p->con_win);
        if (r < 0 || !k) { *key = 0x1B; return 1; }
        if (r == 0) { *key = 0; return 1; }
        if (ev.type == PLUSOS_EV_KEY) {
            if (k->cursor_shown) { t_cursor(k, 0); touch(k); }
            *key = ev.key;
            return 1;
        }
    }
}

/* line editor on a terminal window: 1 if handled, length in *len */
int term_readline(char *buf, int size, int *len)
{
    Proc *p = proc_current;
    if (p->con_win == 0) return 0;
    int n = 0, key;
    if (size <= 0) { *len = 0; return 1; }
    KWin *k = my_term();
    if (!k) { if (p->con_win == 0) return 0; buf[0] = 0; *len = 0; return 1; }
    int had = k->cursor_on;
    k->cursor_on = 1;
    int hcount = kbd_history_count();
    int hpos = hcount;
    int tab_state = 0, tab_base_len = 0;
    char tab_base[128];
    buf[0] = 0;
    for (;;) {
        if (!term_getkey(1, &key)) break;
        k = win_get(p->con_win);
        if (!k) break;
        if (key != '\t') tab_state = 0;
        if (key == '\r' || key == 0x03) { t_putc(k, '\n'); touch(k); break; }
        if (key == 0x1B && p->con_win == -1) break;
        if (key == 0x15 || key == 0x1B) {               /* Ctrl+U or Esc clears input */
            while (n > 0) { n--; t_putc(k, '\b'); }
            buf[0] = 0;
            touch(k);
            continue;
        }
        if (key == 0x08 || key == 0x7F) {
            if (n > 0) { n--; buf[n] = 0; t_putc(k, '\b'); touch(k); }
            continue;
        }
        if (key == '\t') {
            if (tab_state == 0) {
                buf[n] = 0;
                strlcpy(tab_base, buf, sizeof tab_base);
                tab_base_len = n;
            }
            int rc = kbd_tab_complete(tab_base, tab_base_len, buf, size, tab_state, 0, 0);
            if (rc > 0) {
                while (n > 0) { n--; t_putc(k, '\b'); }
                n = (int)strlen(buf);
                for (int i = 0; i < n; i++) t_putc(k, (uint8_t)buf[i]);
                touch(k);
                if (rc == 1) tab_state = 0;
                else tab_state++;
            }
            continue;
        }
        if (key == KEY_UP || key == KEY_DOWN) {
            hcount = kbd_history_count();
            if (!hcount) continue;
            if (key == KEY_UP && hpos > 0) hpos--;
            else if (key == KEY_DOWN && hpos < hcount) hpos++;
            while (n > 0) { n--; t_putc(k, '\b'); }
            buf[0] = 0;
            if (hpos < hcount) {
                const char *h = kbd_history_get(hpos);
                if (h) {
                    strlcpy(buf, h, (size_t)size);
                    n = (int)strlen(buf);
                    for (int i = 0; i < n; i++) t_putc(k, (uint8_t)buf[i]);
                }
            }
            touch(k);
            continue;
        }
        if (key >= 0x20 && key < 0x7F && n < size - 1) {
            buf[n++] = (char)key;
            buf[n] = 0;
            t_putc(k, key);
            touch(k);
        }
    }
    if (k) k->cursor_on = had;
    buf[n] = 0;
    if (n > 0) kbd_history_add(buf);
    *len = n;
    return 1;
}

/* ======================================================================== */
/* syscalls                                                                 */
/* ======================================================================== */
long winsrv_call(struct plusos_win_args *a)
{
    KWin *k;
    switch (a->op) {
    case PLUSOS_WIN_CREATE:
        k = win_create(a->w, a->h, a->title, a->x, a->y, proc_current->pid, 0);
        if (!k) return -1;
        a->bits = k->bits; a->rowbytes = k->rowbytes;
        a->w = k->w; a->h = k->h;
        return k->id;
    case PLUSOS_WIN_UPDATE:
        if (!(k = win_get(a->id))) return -1;
        touch(k);
        return 0;
    case PLUSOS_WIN_EVENT:
        return get_event(a->id, a->ev, a->wait);
    case PLUSOS_WIN_DESTROY:
        if (!(k = win_get(a->id))) return -1;
        win_destroy(k);
        return 0;
    case PLUSOS_WIN_TITLE:
        if (!(k = win_get(a->id))) return -1;
        strlcpy(k->title, a->title ? a->title : "", sizeof k->title);
        touch(k);
        return 0;
    case PLUSOS_WIN_LIST: {
        int n = 0;
        for (int i = 0; i < MAX_KWIN && n < a->max; i++) {
            if (!kw[i].used) continue;
            struct plusos_wininfo *d = &a->list[n++];
            d->id = kw[i].id; d->owner = kw[i].owner; d->term = kw[i].term; d->dead = kw[i].dead;
            d->x = kw[i].x; d->y = kw[i].y; d->w = kw[i].w; d->h = kw[i].h;
            d->seq = kw[i].seq; d->bits = kw[i].bits; d->rowbytes = kw[i].rowbytes;
            strlcpy(d->title, kw[i].title, sizeof d->title);
        }
        a->seq = ws_seq;
        return n;
    }
    case PLUSOS_WIN_POST:
        if (!(k = win_get(a->id)) || !a->ev) return -1;
        post(k, a->ev);
        return 0;
    case PLUSOS_WIN_MOVE:
        if (!(k = win_get(a->id))) return -1;
        k->x = a->x; k->y = a->y;
        return 0;
    }
    return -1;
}

/* Start a program whose console will be a window (opened on first use). */
int winsrv_termspawn(const char *name, int argc, char **argv)
{
    int pid = exec_spawn(name, argc, argv);
    if (pid < 0) return pid;
    for (int i = 0; i < 16; i++) {
        Proc *p = proc_table_entry(i);
        if (p->pid == pid) { p->con_win = -2; break; }
    }
    return pid;
}
