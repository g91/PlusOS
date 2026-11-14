/* ui.c - PlusUI, the PlusOS widget framework (see plusos/ui.h).
 *
 * Everything is drawn with PlusGFX into the form's 1-bit window bitmap. The window
 * server only posts mouse press / release and keys, so while the button is held the
 * framework tracks the pointer itself through the low-memory mouse globals, exactly
 * like the hand-written apps (notes, files, ikanet) do.
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusos/ui.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_H      10              /* text line pitch */
#define SB_W        13              /* scroll bar thickness inside a list */
#define TAB_H       14              /* tab strip height */
#define MENU_ROW    11              /* pull-down / pop-up row height */
#define MAX_OPEN    12              /* the window server's limit */
#define DBL_TICKS   30              /* double-click time */
#define BLINK_TICKS 30

#define MOD_CMD     0x0100
#define MOD_SHIFT   0x0200

static UIForm *open_forms[MAX_OPEN];
static int nopen;
static int quit_flag, quit_code;
static int popup_sticky;            /* a menu opened by a click stays up until the next click */

/* ======================================================================== */
/* small helpers                                                            */
/* ======================================================================== */
static int mouse_btn(void) { return (*(volatile uint8_t *)0x172 & 0x80) == 0; }
static int raw_mx(void)    { return *(volatile int16_t *)0x832; }
static int raw_my(void)    { return *(volatile int16_t *)0x830; }

/* pointer position in surface coordinates */
static void mouse_surf(UIForm *f, int *x, int *y) { *x = raw_mx() - f->ox; *y = raw_my() - f->oy; }
/* pointer position in client coordinates */
static void mouse_pos(UIForm *f, int *x, int *y) { mouse_surf(f, x, y); *y -= f->top; }

static void nap(int t) { if (plusos->sleep_ticks) plusos->sleep_ticks(t); }
static uint32_t now(void) { return plusos->ticks ? plusos->ticks() : 0; }
static int mods(void) { return plusos->key_mods ? plusos->key_mods() : 0; }

static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }
static int clampi(int v, int lo, int hi) { if (v > hi) v = hi; if (v < lo) v = lo; return v; }

static char *dupstr(const char *s)
{
    if (!s) s = "";
    size_t n = strlen(s);
    char *d = malloc(n + 1);
    if (d) memcpy(d, s, n + 1);
    return d;
}

static int is_edit(const UIWidget *w) { return w->kind == UI_TEXTFIELD || w->kind == UI_TEXTAREA; }
static int focusable(const UIWidget *w) { return is_edit(w) || w->kind == UI_LISTBOX; }

static int visible(const UIWidget *w)
{
    for (int depth = 0; w && depth < 8; depth++) {
        if (w->flags & UI_F_HIDDEN) return 0;
        if (!w->tabs) return 1;
        if (w->tabs->value != w->tab_page) return 0;
        w = w->tabs;
    }
    return 1;
}
static int usable(const UIWidget *w) { return visible(w) && !(w->flags & UI_F_DISABLED); }

/* narrow the clip to a rectangle, remembering the old one */
static void clip_in(int l, int t, int r, int b, GRect *save)
{
    GRect n;
    gfx_get_clip(save);
    n.l = (int16_t)imax(l, save->l); n.t = (int16_t)imax(t, save->t);
    n.r = (int16_t)imin(r, save->r); n.b = (int16_t)imin(b, save->b);
    if (n.r < n.l) n.r = n.l;
    if (n.b < n.t) n.b = n.t;
    gfx_set_clip(&n);
}

static void text_n(int x, int y, const char *s, int n, int inv)
{
    for (int i = 0; i < n && s[i]; i++, x += 8) gfx_char(x, y, (uint8_t)s[i], inv);
}

static void present(UIForm *f)
{
    if (f->opened) gwin_update(&f->win);
    else if (f->on_present) f->on_present(f);
}

static void redraw_now(UIWidget *w) { ui_invalidate(w); ui_form_draw(w->form); }

/* ======================================================================== */
/* scroll bars (the widget, and the one inside a list)                      */
/* ======================================================================== */
typedef struct { int l, t, r, b, vert, min, max, page, value; } SB;

static void sb_metrics(const SB *s, int *a0, int *a1, int *th0, int *th1)
{
    int th = s->vert ? s->r - s->l : s->b - s->t;           /* arrow box size */
    int lo = s->vert ? s->t : s->l, hi = s->vert ? s->b : s->r;
    if (hi - lo < 3 * th) th = (hi - lo) / 3;
    *a0 = lo + th; *a1 = hi - th;
    int track = *a1 - *a0, range = s->max - s->min;
    if (range <= 0 || track <= 0) { *th0 = *th1 = *a0; return; }
    int len = s->page > 0 ? (int)((long)track * s->page / (range + s->page)) : 12;
    if (len < 8) len = 8;
    if (len > track) len = track;
    int off = (int)((long)(track - len) * (s->value - s->min) / range);
    *th0 = *a0 + off; *th1 = *th0 + len;
}

/* a small solid triangle: dir 0 up, 1 down, 2 left, 3 right */
static void tri(int cx, int cy, int dir)
{
    for (int i = 0; i < 4; i++) {
        if (dir == 0)      gfx_hline(cx - i, cx + i + 1, cy - 2 + i, 1);
        else if (dir == 1) gfx_hline(cx - i, cx + i + 1, cy + 1 - i, 1);
        else if (dir == 2) gfx_vline(cx - 2 + i, cy - i, cy + i + 1, 1);
        else               gfx_vline(cx + 1 - i, cy - i, cy + i + 1, 1);
    }
}

static void sb_draw(const SB *s)
{
    int a0, a1, th0, th1;
    sb_metrics(s, &a0, &a1, &th0, &th1);
    gfx_fill(s->l, s->t, s->r, s->b, 0);
    if (s->vert) {
        int cx = (s->l + s->r - 1) / 2;
        if (th1 > th0) {
            gfx_gray(s->l + 1, a0, s->r - 1, a1);
            gfx_fill(s->l + 1, th0, s->r - 1, th1, 0);
            gfx_hline(s->l, s->r, th0, 1); gfx_hline(s->l, s->r, th1 - 1, 1);
        }
        gfx_hline(s->l, s->r, a0 - 1, 1); gfx_hline(s->l, s->r, a1, 1);
        tri(cx, (s->t + a0) / 2, 0); tri(cx, (a1 + s->b) / 2, 1);
    } else {
        int cy = (s->t + s->b - 1) / 2;
        if (th1 > th0) {
            gfx_gray(a0, s->t + 1, a1, s->b - 1);
            gfx_fill(th0, s->t + 1, th1, s->b - 1, 0);
            gfx_vline(th0, s->t, s->b, 1); gfx_vline(th1 - 1, s->t, s->b, 1);
        }
        gfx_vline(a0 - 1, s->t, s->b, 1); gfx_vline(a1, s->t, s->b, 1);
        tri((s->l + a0) / 2, cy, 2); tri((a1 + s->r) / 2, cy, 3);
    }
    gfx_frame(s->l, s->t, s->r, s->b);
}

static void sb_apply(UIWidget *w, int *val, int v)
{
    if (*val == v) return;
    *val = v;
    if (w->kind == UI_SCROLLBAR && w->on_change) w->on_change(w);
    redraw_now(w);
}

/* the button went down at surface point (sx, sy) inside the bar: follow it */
static void sb_track(UIWidget *w, SB *s, int sx, int sy, int *val)
{
    UIForm *f = w->form;
    int a0, a1, th0, th1, range = s->max - s->min;
    if (range <= 0) return;
    s->value = *val;
    sb_metrics(s, &a0, &a1, &th0, &th1);
    int pos = s->vert ? sy : sx;
    int pg = s->page > 0 ? s->page : imax(range / 10, 1);
    if (pos >= th0 && pos < th1) {                      /* drag the thumb */
        int grab = pos - th0, len = th1 - th0, room = (a1 - a0) - len;
        while (mouse_btn()) {
            int mx, my;
            mouse_surf(f, &mx, &my);
            int off = clampi((s->vert ? my : mx) - grab - a0, 0, room);
            int v = room > 0 ? s->min + (int)(((long)off * range + room / 2) / room) : s->min;
            sb_apply(w, val, v);
            nap(1);
        }
        return;
    }
    int delta = pos < a0 ? -1 : pos >= a1 ? 1 : pos < th0 ? -pg : pg;
    int first = 1;
    do {                                                /* arrows and page areas repeat */
        sb_apply(w, val, clampi(*val + delta, s->min, s->max));
        for (int i = 0; i < (first ? 16 : 3) && mouse_btn(); i++) nap(1);
        first = 0;
    } while (mouse_btn());
}

/* ======================================================================== */
/* edit boxes                                                               */
/* ======================================================================== */
static int ta_cols(const UIWidget *w) { int c = (w->w - 8) / 8; return c < 1 ? 1 : c; }
static int ta_rows(const UIWidget *w) { int r = (w->h - 6) / LINE_H; return r < 1 ? 1 : r; }

static void ta_locate(const UIWidget *w, int pos, int *line, int *col)
{
    int cols = ta_cols(w), ln = 0, c = 0;
    const char *s = w->text;
    for (int i = 0; i < pos && s[i]; i++) {
        if (s[i] == '\n') { ln++; c = 0; }
        else if (++c >= cols) { ln++; c = 0; }
    }
    *line = ln; *col = c;
}

static int ta_pos(const UIWidget *w, int line, int col)
{
    int cols = ta_cols(w), ln = 0, c = 0, i;
    const char *s = w->text;
    if (line < 0) return 0;
    for (i = 0; s[i]; i++) {
        if (ln == line && c >= col) return i;
        if (s[i] == '\n') { if (ln == line) return i; ln++; c = 0; }
        else if (++c >= cols) { if (ln == line) return i; ln++; c = 0; }
    }
    return i;
}

/* replace [a, b) with n characters of ins; returns the number inserted */
static int ed_replace(UIWidget *w, int a, int b, const char *ins, int n)
{
    int len = (int)strlen(w->text);
    a = clampi(a, 0, len); b = clampi(b, a, len);
    if (n > w->cap - (len - (b - a))) n = w->cap - (len - (b - a));
    if (n < 0) n = 0;
    memmove(w->text + a + n, w->text + b, (size_t)(len - b + 1));
    if (n) memcpy(w->text + a, ins, (size_t)n);
    w->caret = w->anchor = a + n;
    return n;
}

static void ed_changed(UIWidget *w)
{
    if (w->on_change) w->on_change(w);
    ui_invalidate(w);
}

static void caret_show(UIForm *f) { f->caret_on = 1; f->blink_next = now() + BLINK_TICKS; }

/* returns 1 when the key was used */
static int edit_key(UIWidget *w, int key)
{
    int len = (int)strlen(w->text);
    int m = mods(), shift = m & MOD_SHIFT;
    int a = imin(w->caret, w->anchor), b = imax(w->caret, w->anchor);
    int ro = (w->flags & UI_F_READONLY) != 0;
    int line, col;

    caret_show(w->form);
    if (m & MOD_CMD) {
        int k = key | 0x20;
        if (k == 'a') { w->anchor = 0; w->caret = len; ui_invalidate(w); return 1; }
        if (k == 'c' || k == 'x') {
            if (b > a && !(w->flags & UI_F_PASSWORD)) ui_clip_set(w->text + a, b - a);
            if (k == 'x' && b > a && !ro) { ed_replace(w, a, b, "", 0); ed_changed(w); }
            return 1;
        }
        if (k == 'v') {
            char buf[256];
            int n = ro ? 0 : ui_clip_get(buf, sizeof buf);
            if (n > 0) {
                if (w->kind == UI_TEXTFIELD)
                    for (int i = 0; i < n; i++) if (buf[i] == '\n' || buf[i] == '\r') { n = i; break; }
                for (int i = 0; i < n; i++) if (buf[i] == '\r') buf[i] = '\n';
                ed_replace(w, a, b, buf, n);
                ed_changed(w);
            }
            return 1;
        }
        return 0;
    }
    switch (key) {
    case KEY_LEFT:
        if (!shift && a != b) w->caret = a; else if (w->caret > 0) w->caret--;
        break;
    case KEY_RIGHT:
        if (!shift && a != b) w->caret = b; else if (w->caret < len) w->caret++;
        break;
    case KEY_HOME:
        if (w->kind == UI_TEXTAREA) { ta_locate(w, w->caret, &line, &col); w->caret = ta_pos(w, line, 0); }
        else w->caret = 0;
        break;
    case KEY_END:
        if (w->kind == UI_TEXTAREA) { ta_locate(w, w->caret, &line, &col); w->caret = ta_pos(w, line, 9999); }
        else w->caret = len;
        break;
    case KEY_UP: case KEY_DOWN: case KEY_PGUP: case KEY_PGDN:
        if (w->kind != UI_TEXTAREA) return 0;
        ta_locate(w, w->caret, &line, &col);
        line += key == KEY_UP ? -1 : key == KEY_DOWN ? 1 : key == KEY_PGUP ? -ta_rows(w) : ta_rows(w);
        w->caret = line < 0 ? 0 : ta_pos(w, line, col);
        break;
    case KEY_BACKSPACE:
        if (ro) return 1;
        if (a == b) { if (!a) return 1; a--; }
        ed_replace(w, a, b, "", 0);
        ed_changed(w);
        return 1;
    case KEY_DELETE:
        if (ro) return 1;
        if (a == b) { if (b >= len) return 1; b++; }
        ed_replace(w, a, b, "", 0);
        ed_changed(w);
        return 1;
    case KEY_RETURN: case KEY_ENTER:
        if (w->kind != UI_TEXTAREA) return 0;
        if (!ro) { ed_replace(w, a, b, "\n", 1); ed_changed(w); }
        return 1;
    default:
        if (key < 0x20 || key > 0x7E) return 0;
        if (!ro) { char c = (char)key; ed_replace(w, a, b, &c, 1); ed_changed(w); }
        return 1;
    }
    if (!shift) w->anchor = w->caret;
    ui_invalidate(w);
    return 1;
}

/* the character position under a client-coordinate point */
static int edit_hit(const UIWidget *w, int x, int y)
{
    int len = (int)strlen(w->text);
    int col = (x - (w->x + 4) + 4) / 8;
    if (col < 0) col = 0;
    if (w->kind == UI_TEXTFIELD) return clampi(w->scroll + col, 0, len);
    int line = w->scroll + (y - (w->y + 3)) / LINE_H;
    if (y < w->y + 3) line = w->scroll - 1;
    return line < 0 ? 0 : ta_pos(w, line, col);
}

/* ======================================================================== */
/* drawing the widgets                                                      */
/* ======================================================================== */
static void draw_label_text(const UIWidget *w, int l, int t, int r, int b)
{
    const char *s = w->text;
    int nlines = 1;
    for (const char *p = s; *p; p++) if (*p == '\n') nlines++;
    int y = t + ((b - t) - (nlines * LINE_H - 2)) / 2;
    if (y < t) y = t;
    while (1) {
        int n = 0;
        while (s[n] && s[n] != '\n') n++;
        int x = l;
        if (w->flags & UI_F_CENTER) x = l + ((r - l) - n * 8) / 2;
        else if (w->flags & UI_F_RIGHT) x = r - n * 8;
        text_n(x, y, s, n, 0);
        if (!s[n]) break;
        s += n + 1; y += LINE_H;
    }
}

static void draw_button(const UIWidget *w, int l, int t, int r, int b)
{
    int n = (int)strlen(w->text);
    if (w->flags & UI_F_DEFAULT) {
        gfx_round_rect(l, t, r, b, 6, 1);
        gfx_round_rect(l + 1, t + 1, r - 1, b - 1, 5, 1);
        l += 3; t += 3; r -= 3; b -= 3;
    }
    gfx_fill_round_rect(l, t, r, b, 4, w->pressed ? 1 : 0);
    gfx_round_rect(l, t, r, b, 4, 1);
    text_n(l + ((r - l) - n * 8) / 2, t + ((b - t) - 8) / 2, w->text, n, w->pressed);
}

static void draw_field(UIWidget *w, int l, int t, int r, int b)
{
    UIForm *f = w->form;
    GRect save;
    int len = (int)strlen(w->text);
    int cols = imax((w->w - 8) / 8, 1);
    int focused = f->focus == w;
    int a = imin(w->caret, w->anchor), e = imax(w->caret, w->anchor);
    int y = t + ((b - t) - 8) / 2;

    if (w->caret > len) w->caret = len;
    if (w->anchor > len) w->anchor = len;
    if (w->caret < w->scroll) w->scroll = w->caret;
    if (w->caret > w->scroll + cols) w->scroll = w->caret - cols;
    if (w->scroll > len) w->scroll = len;
    gfx_fill(l, t, r, b, 0);
    gfx_frame(l, t, r, b);
    clip_in(l + 2, t + 1, r - 2, b - 1, &save);
    for (int i = w->scroll; i < len && i <= w->scroll + cols; i++) {
        int ch = (w->flags & UI_F_PASSWORD) ? '*' : (uint8_t)w->text[i];
        gfx_char(l + 4 + (i - w->scroll) * 8, y, ch, focused && i >= a && i < e);
    }
    if (focused && f->caret_on && a == e)
        gfx_vline(l + 3 + (w->caret - w->scroll) * 8, y - 1, y + 9, 1);
    gfx_set_clip(&save);
}

static void draw_area(UIWidget *w, int l, int t, int r, int b)
{
    UIForm *f = w->form;
    GRect save;
    const char *s = w->text;
    int len = (int)strlen(s), cols = ta_cols(w), rows = ta_rows(w);
    int focused = f->focus == w;
    int a = imin(w->caret, w->anchor), e = imax(w->caret, w->anchor);
    int cline, ccol, ln = 0, c = 0;

    if (w->caret > len) w->caret = len;
    if (w->anchor > len) w->anchor = len;
    ta_locate(w, w->caret, &cline, &ccol);
    if (cline < w->scroll) w->scroll = cline;
    if (cline >= w->scroll + rows) w->scroll = cline - rows + 1;
    gfx_fill(l, t, r, b, 0);
    gfx_frame(l, t, r, b);
    clip_in(l + 2, t + 2, r - 2, b - 2, &save);
    for (int i = 0; i < len && ln < w->scroll + rows; i++) {
        if (s[i] == '\n') { ln++; c = 0; continue; }
        if (ln >= w->scroll)
            gfx_char(l + 4 + c * 8, t + 3 + (ln - w->scroll) * LINE_H, (uint8_t)s[i], focused && i >= a && i < e);
        if (++c >= cols) { ln++; c = 0; }
    }
    if (focused && f->caret_on && a == e) {
        int y = t + 3 + (cline - w->scroll) * LINE_H;
        gfx_vline(l + 3 + ccol * 8, y - 1, y + 9, 1);
    }
    gfx_set_clip(&save);
}

static int list_rows(const UIWidget *w) { int r = (w->h - 2) / LINE_H; return r < 1 ? 1 : r; }
static int list_has_sb(const UIWidget *w) { return w->nitems > list_rows(w); }

static void list_sb(const UIWidget *w, SB *s)
{
    int top = w->form->top;
    s->l = w->x + w->w - SB_W; s->t = w->y + top; s->r = w->x + w->w; s->b = w->y + w->h + top;
    s->vert = 1; s->min = 0; s->max = imax(w->nitems - list_rows(w), 0);
    s->page = list_rows(w); s->value = w->scroll;
}

static void draw_list(UIWidget *w, int l, int t, int r, int b)
{
    GRect save;
    int rows = list_rows(w), sbw = list_has_sb(w) ? SB_W - 1 : 0;
    w->scroll = clampi(w->scroll, 0, imax(w->nitems - rows, 0));
    gfx_fill(l, t, r, b, 0);
    gfx_frame(l, t, r, b);
    clip_in(l + 1, t + 1, r - 1 - sbw, b - 1, &save);
    for (int i = 0; i < rows && w->scroll + i < w->nitems; i++) {
        int idx = w->scroll + i, y = t + 1 + i * LINE_H;
        int sel = idx == w->value;
        if (sel) gfx_fill(l + 1, y, r - 1 - sbw, y + LINE_H, 1);
        text_n(l + 4, y + 1, w->items[idx], 255, sel);
    }
    gfx_set_clip(&save);
    if (sbw) { SB s; list_sb(w, &s); sb_draw(&s); }
    if (w->form->focus == w) gfx_frame(l + 1, t + 1, r - 1 - sbw, b - 1);
}

static void draw_combo(const UIWidget *w, int l, int t, int r, int b)
{
    GRect save;
    int cy = t + (b - t) / 2;
    gfx_fill(l, t, r, b, 0);
    gfx_frame(l, t, r, b);
    gfx_hline(l + 2, r + 1, b, 1); gfx_vline(r, t + 2, b + 1, 1);       /* drop shadow (inside the clip only) */
    gfx_vline(r - 15, t, b, 1);
    tri(r - 8, cy, 1);
    if (w->value >= 0 && w->value < w->nitems) {
        clip_in(l + 2, t + 1, r - 16, b - 1, &save);
        text_n(l + 4, t + ((b - t) - 8) / 2, w->items[w->value], 255, 0);
        gfx_set_clip(&save);
    }
}

static void draw_slider(const UIWidget *w, int l, int t, int r, int b)
{
    int range = imax(w->max - w->min, 1), v = clampi(w->value, w->min, w->max) - w->min;
    if (w->h > w->w) {                                  /* vertical: the bottom is min */
        int cx = (l + r) / 2, room = (b - t) - 9;
        int ky = b - 9 - (int)((long)room * v / range);
        gfx_fill(cx - 1, t, cx + 2, b, 0); gfx_frame(cx - 1, t, cx + 2, b);
        gfx_fill(l, ky, r, ky + 9, 0); gfx_frame(l, ky, r, ky + 9);
        gfx_hline(l + 2, r - 2, ky + 4, 1);
    } else {
        int cy = (t + b) / 2, room = (r - l) - 9;
        int kx = l + (int)((long)room * v / range);
        gfx_fill(l, cy - 1, r, cy + 2, 0); gfx_frame(l, cy - 1, r, cy + 2);
        gfx_fill(kx, t, kx + 9, b, 0); gfx_frame(kx, t, kx + 9, b);
        gfx_vline(kx + 4, t + 2, b - 2, 1);
    }
}

static void widget_sb(const UIWidget *w, SB *s)
{
    int top = w->form->top;
    s->l = w->x; s->t = w->y + top; s->r = w->x + w->w; s->b = w->y + w->h + top;
    s->vert = w->h > w->w; s->min = w->min; s->max = w->max; s->page = w->page;
    s->value = clampi(w->value, w->min, w->max);
}

static int tab_at(const UIWidget *w, int x)     /* x relative to the widget */
{
    int tx = 4;
    for (int i = 0; i < w->nitems; i++) {
        int tw = (int)strlen(w->items[i]) * 8 + 12;
        if (x >= tx && x < tx + tw) return i;
        tx += tw;
    }
    return -1;
}

static void draw_tabs(const UIWidget *w, int l, int t, int r, int b)
{
    int tx = l + 4;
    gfx_fill(l, t + TAB_H - 1, r, b, 0);
    gfx_frame(l, t + TAB_H - 1, r, b);
    for (int i = 0; i < w->nitems; i++) {
        int n = (int)strlen(w->items[i]), tw = n * 8 + 12;
        if (i == w->value) {
            gfx_fill(tx, t, tx + tw, t + TAB_H, 0);
            gfx_frame(tx, t, tx + tw, t + TAB_H);
            gfx_hline(tx + 1, tx + tw - 1, t + TAB_H - 1, 0);
            text_n(tx + 6, t + 3, w->items[i], n, 0);
        } else {
            gfx_frame(tx, t + 2, tx + tw, t + TAB_H);
            text_n(tx + 6, t + 4, w->items[i], n, 0);
        }
        tx += tw;
    }
}

static void draw_image(const UIWidget *w, int l, int t, int r, int b)
{
    if (w->bits && w->bw > 0 && w->bh > 0) {
        GSurface s;
        gs_wrap(&s, (void *)w->bits, ((w->bw + 15) >> 4) << 1, w->bw, w->bh);
        gfx_blit(&s, 0, 0, imin(w->bw, r - l), imin(w->bh, b - t), l, t, GFX_COPY);
    }
    if (w->pressed) gfx_invert(l, t, r, b);
}

static void draw_widget(UIWidget *w)
{
    UIForm *f = w->form;
    int l = w->x, t = w->y + f->top, r = l + w->w, b = t + w->h;
    int n;

    w->dirty = 0;
    switch (w->kind) {
    case UI_LABEL:
        draw_label_text(w, l, t, r, b);
        if (w->flags & UI_F_BORDER) gfx_frame(l, t, r, b);
        break;
    case UI_BUTTON:
        draw_button(w, l, t, r, b);
        break;
    case UI_CHECKBOX: {
        int by = t + ((b - t) - 12) / 2;
        gfx_fill(l, by, l + 12, by + 12, 0);
        gfx_frame(l, by, l + 12, by + 12);
        if (w->pressed) gfx_frame(l + 1, by + 1, l + 11, by + 11);
        if (w->value) { gfx_line(l + 2, by + 2, l + 9, by + 9, 1); gfx_line(l + 9, by + 2, l + 2, by + 9, 1); }
        gfx_text(l + 18, t + ((b - t) - 8) / 2, w->text, 0);
        break;
    }
    case UI_RADIO: {
        int cy = t + (b - t) / 2;
        gfx_fill_circle(l + 6, cy, 5, 0);
        gfx_circle(l + 6, cy, 5, 1);
        if (w->pressed) gfx_circle(l + 6, cy, 4, 1);
        if (w->value) gfx_fill_circle(l + 6, cy, 2, 1);
        gfx_text(l + 18, t + ((b - t) - 8) / 2, w->text, 0);
        break;
    }
    case UI_TEXTFIELD: draw_field(w, l, t, r, b); break;
    case UI_TEXTAREA:  draw_area(w, l, t, r, b); break;
    case UI_LISTBOX:   draw_list(w, l, t, r, b); break;
    case UI_COMBO:     draw_combo(w, l, t, r - 1, b - 1); break;
    case UI_SLIDER:    draw_slider(w, l, t, r, b); break;
    case UI_SCROLLBAR: { SB s; widget_sb(w, &s); sb_draw(&s); break; }
    case UI_PROGRESS: {
        int range = imax(w->max - w->min, 1), v = clampi(w->value, w->min, w->max) - w->min;
        gfx_fill(l, t, r, b, 0);
        gfx_frame(l, t, r, b);
        gfx_fill(l + 2, t + 2, l + 2 + (int)((long)(w->w - 4) * v / range), b - 2, 1);
        break;
    }
    case UI_GROUP:
        gfx_frame(l, t + 4, r, b);
        n = (int)strlen(w->text);
        if (n) { gfx_fill(l + 6, t, l + 10 + n * 8, t + 8, 0); gfx_text(l + 8, t, w->text, 0); }
        break;
    case UI_IMAGE:
        draw_image(w, l, t, r, b);
        if (w->flags & UI_F_BORDER) gfx_frame(l, t, r, b);
        break;
    case UI_SEPARATOR:
        if (w->h > w->w) gfx_vline(l + w->w / 2, t, b, 1);
        else gfx_hline(l, r, t + w->h / 2, 1);
        break;
    case UI_TABS:
        draw_tabs(w, l, t, r, b);
        break;
    case UI_CANVAS: {
        GRect save;
        clip_in(l, t, r, b, &save);
        if (w->on_draw) w->on_draw(w, l, t, r, b);
        gfx_set_clip(&save);
        if (w->flags & UI_F_BORDER) gfx_frame(l, t, r, b);
        break;
    }
    }
    if (w->flags & UI_F_DISABLED) gfx_dim(l, t, r, b);
}

/* ======================================================================== */
/* menus and pop-ups                                                        */
/* ======================================================================== */
/* "Quit^Q" -> text length 4, shortcut 'Q' */
static int item_shortcut(const char *s, int *textlen)
{
    int n = (int)strlen(s);
    if (n >= 3 && s[n - 2] == '^') { *textlen = n - 2; return (uint8_t)s[n - 1]; }
    *textlen = n;
    return 0;
}

static int menu_title_at(const UIForm *f, int sx, int *l, int *r)
{
    int x = 4;
    for (int i = 0; i < f->nmenus; i++) {
        int w = (int)strlen(f->menus[i].title) * 8 + 16;
        if (sx >= x && sx < x + w) { if (l) *l = x; if (r) *r = x + w; return i; }
        x += w;
    }
    return -1;
}

static void menubar_draw(const UIForm *f, int hot)
{
    int x = 4, W = f->surf->w;
    gfx_fill(0, 0, W, f->top, 0);
    gfx_hline(0, W, f->top - 1, 1);
    for (int i = 0; i < f->nmenus; i++) {
        const char *s = f->menus[i].title;
        int w = (int)strlen(s) * 8 + 16;
        if (i == hot) gfx_fill(x, 0, x + w, f->top - 1, 1);
        gfx_text(x + 8, 2, s, i == hot);
        x += w;
    }
}

/* Run a pop-up list at surface (x, y). The pointer starts inside the anchor rect
 * (the menu title or combo box). Returns the item picked, -1 for none, or
 * -100 - m when the pointer moved to menu title m (menus only). */
static int popup_run(UIForm *f, int x, int y, int minw, const char *const *items, int n, int cur,
                     int al, int at, int ar, int ab, int is_menu)
{
    int SW = f->surf->w, SH = f->surf->h;
    int pw = minw, vis, ph, top = 0, hot = -2, i;

    for (i = 0; i < n; i++) {
        int tl, sc = is_menu ? item_shortcut(items[i], &tl) : (tl = (int)strlen(items[i]), 0);
        int w = tl * 8 + 16 + (sc ? 32 : 0);
        if (w > pw) pw = w;
    }
    if (pw > SW - 2) pw = SW - 2;
    vis = imin(n, imax((SH - 4) / MENU_ROW, 1));
    ph = vis * MENU_ROW + 2;
    if (x + pw + 2 > SW) x = SW - pw - 2;
    if (x < 0) x = 0;
    if (y + ph + 2 > SH) y = SH - ph - 2;
    if (y < 0) y = 0;
    if (cur >= vis) top = cur - vis + 1;

    gfx_target(f->surf);
    for (;;) {
        int mx, my, hit = -1, down = mouse_btn(), scrolled = 0;
        mouse_surf(f, &mx, &my);
        if (mx >= x && mx < x + pw && my >= y + 1 && my < y + ph - 1) {
            hit = top + (my - y - 1) / MENU_ROW;
            if (hit >= n || (is_menu && !strcmp(items[hit], "-"))) hit = -1;
        } else if (mx >= x && mx < x + pw) {                    /* past an end: scroll */
            if (my >= y + ph - 1 && top + vis < n) { top++; scrolled = 1; }
            if (my < y + 1 && top > 0 && !(my >= at && my < ab)) { top--; scrolled = 1; }
        }
        if (is_menu && my >= 0 && my < f->top) {
            int m = menu_title_at(f, mx, 0, 0);
            if (m >= 0 && (mx < al || mx >= ar)) return -100 - m;
        }
        if (hit != hot || scrolled) {
            hot = hit;
            gfx_fill(x + 2, y + 2, x + pw + 2, y + ph + 2, 1);
            gfx_fill(x, y, x + pw, y + ph, 0);
            gfx_frame(x, y, x + pw, y + ph);
            for (i = 0; i < vis; i++) {
                int idx = top + i, ry = y + 1 + i * MENU_ROW, tl, sc = 0;
                const char *s = items[idx];
                if (is_menu && !strcmp(s, "-")) { gfx_pattern(x + 1, ry + 5, x + pw - 1, ry + 6, 0xAA, 0xAA); continue; }
                if (is_menu) sc = item_shortcut(s, &tl); else tl = (int)strlen(s);
                if (idx == hot) gfx_fill(x + 1, ry, x + pw - 1, ry + MENU_ROW, 1);
                text_n(x + 8, ry + 2, s, imin(tl, (pw - 12) / 8), idx == hot);
                if (sc) { gfx_char(x + pw - 24, ry + 2, '^', idx == hot); gfx_char(x + pw - 16, ry + 2, sc, idx == hot); }
            }
            present(f);
        }
        if (!popup_sticky) {
            if (!down) {
                if (hot >= 0) return hot;
                /* a plain click on the anchor leaves the list up until the next click */
                if (f->opened && mx >= al && mx < ar && my >= at && my < ab) { popup_sticky = 1; continue; }
                return -1;
            }
        } else {
            GEvent ev;
            int r = gwin_event(&f->win, &ev, 0);
            if (r < 0) return -1;
            if (r > 0) {
                if (ev.type == GEV_KEY && ev.key == KEY_ESC) return -1;
                if (ev.type == GEV_CLOSE) return -1;
                if (ev.type == GEV_MOUSEDOWN) {
                    f->ox = raw_mx() - ev.x; f->oy = raw_my() - ev.y;
                    while (mouse_btn()) nap(1);
                    if (is_menu && ev.y < f->top) {
                        int m = menu_title_at(f, ev.x, 0, 0);
                        if (m >= 0 && (ev.x < al || ev.x >= ar)) return -100 - m;
                    }
                    return hot;
                }
            }
        }
        nap(scrolled ? 4 : 1);
    }
}

static void form_paint(UIForm *f);

static void menu_track(UIForm *f, int sx)
{
    int l, r, res = -1;
    int m = menu_title_at(f, sx, &l, &r);
    if (m < 0) return;
    popup_sticky = 0;
    for (;;) {
        int x = 4;
        for (int i = 0; i < m; i++) x += (int)strlen(f->menus[i].title) * 8 + 16;
        l = x; r = x + (int)strlen(f->menus[m].title) * 8 + 16;
        f->dirty_all = 1;
        form_paint(f);
        gfx_target(f->surf);
        menubar_draw(f, m);
        res = popup_run(f, l, f->top - 1, r - l, f->menus[m].items, f->menus[m].nitems, -1,
                        l, 0, r, f->top, 1);
        if (res > -100) break;
        m = -100 - res;
    }
    popup_sticky = 0;
    f->dirty_all = 1;
    ui_form_draw(f);
    if (res >= 0 && f->on_menu) f->on_menu(f, m, res);
}

static int menu_shortcut(UIForm *f, int key)
{
    int k = key >= 'a' && key <= 'z' ? key - 32 : key;
    for (int m = 0; m < f->nmenus; m++)
        for (int i = 0; i < f->menus[m].nitems; i++) {
            int tl, sc = item_shortcut(f->menus[m].items[i], &tl);
            if (sc >= 'a' && sc <= 'z') sc -= 32;
            if (sc && sc == k) { if (f->on_menu) f->on_menu(f, m, i); return 1; }
        }
    return 0;
}

/* ======================================================================== */
/* painting a form                                                          */
/* ======================================================================== */
static void form_paint(UIForm *f)
{
    GSurface *old;
    int W, H, i, j;
    if (!f->surf || f->in_draw) return;
    f->in_draw = 1;
    old = gfx_get_target();
    gfx_target(f->surf);
    W = f->surf->w; H = f->surf->h;
    if (f->dirty_all) {
        gfx_fill(0, 0, W, H, 0);
        if (f->top) menubar_draw(f, -1);
        gfx_clip(0, f->top, W, H);
        for (i = 0; i < f->nw; i++) {
            f->wd[i]->dirty = 0;
            if (visible(f->wd[i])) draw_widget(f->wd[i]);
        }
    } else {
        for (i = 0; i < f->nw; i++) {
            UIWidget *w = f->wd[i];
            if (!w->dirty) continue;
            w->dirty = 0;
            if (!visible(w)) continue;
            int l = w->x, t = w->y + f->top, r = l + w->w, b = t + w->h;
            gfx_clip(l, imax(t, f->top), r, b);
            gfx_fill(l, t, r, b, 0);
            for (j = 0; j < f->nw; j++) {               /* everything that overlaps, in z order */
                UIWidget *v = f->wd[j];
                if (!visible(v)) continue;
                if (v->x >= w->x + w->w || v->x + v->w <= w->x || v->y >= w->y + w->h || v->y + v->h <= w->y) continue;
                int was = v->dirty;
                draw_widget(v);
                v->dirty = (uint8_t)(j > i ? was : 0);
            }
        }
    }
    gfx_noclip();
    f->dirty_all = 0; f->any_dirty = 0;
    if (old != f->surf) gfx_target(old);
    f->in_draw = 0;
}

void ui_form_draw(UIForm *f)
{
    if (!f || !f->surf) return;
    if (!f->dirty_all && !f->any_dirty) return;
    form_paint(f);
    present(f);
}

void ui_form_invalidate(UIForm *f) { if (f) f->dirty_all = 1; }

void ui_invalidate(UIWidget *w)
{
    if (!w) return;
    w->dirty = 1;
    if (w->form) w->form->any_dirty = 1;
}

/* ======================================================================== */
/* focus, keys                                                              */
/* ======================================================================== */
void ui_focus(UIWidget *w)
{
    UIForm *f;
    if (!w) return;
    f = w->form;
    if (f->focus == w) return;
    if (f->focus) {
        if (is_edit(f->focus)) f->focus->anchor = f->focus->caret;
        ui_invalidate(f->focus);
    }
    f->focus = w;
    caret_show(f);
    ui_invalidate(w);
}

static void focus_next(UIForm *f, int dir)
{
    int cur = -1, i;
    for (i = 0; i < f->nw; i++) if (f->wd[i] == f->focus) cur = i;
    for (i = 1; i <= f->nw; i++) {
        int k = cur < 0 ? (dir > 0 ? i - 1 : f->nw - i) : (cur + dir * i + f->nw * 2) % f->nw;
        UIWidget *w = f->wd[k];
        if (focusable(w) && usable(w)) {
            ui_focus(w);
            if (w->kind == UI_TEXTFIELD) ui_select_all(w);
            return;
        }
    }
}

static UIWidget *find_flag_button(UIForm *f, unsigned flag)
{
    for (int i = 0; i < f->nw; i++)
        if (f->wd[i]->kind == UI_BUTTON && (f->wd[i]->flags & flag) && usable(f->wd[i])) return f->wd[i];
    return 0;
}

static void press_button(UIWidget *w)
{
    w->pressed = 1; redraw_now(w);
    nap(6);
    w->pressed = 0; redraw_now(w);
    if (w->on_action) w->on_action(w);
}

static void list_select(UIWidget *w, int idx)
{
    int rows = list_rows(w);
    idx = clampi(idx, w->nitems ? 0 : -1, w->nitems - 1);
    if (idx >= 0) {
        if (idx < w->scroll) w->scroll = idx;
        if (idx >= w->scroll + rows) w->scroll = idx - rows + 1;
    }
    ui_invalidate(w);
    if (idx == w->value) return;
    w->value = idx;
    if (w->on_change) w->on_change(w);
}

static int list_key(UIWidget *w, int key)
{
    switch (key) {
    case KEY_UP:    list_select(w, w->value <= 0 ? 0 : w->value - 1); return 1;
    case KEY_DOWN:  list_select(w, w->value + 1); return 1;
    case KEY_PGUP:  list_select(w, imax(w->value - list_rows(w), 0)); return 1;
    case KEY_PGDN:  list_select(w, w->value + list_rows(w)); return 1;
    case KEY_HOME:  list_select(w, 0); return 1;
    case KEY_END:   list_select(w, w->nitems - 1); return 1;
    case KEY_RETURN: case KEY_ENTER:
        if (w->on_action && w->value >= 0) { w->on_action(w); return 1; }
        return 0;
    }
    return 0;
}

static void form_key(UIForm *f, int key)
{
    UIWidget *fw = f->focus, *b;
    int is_ret = key == KEY_RETURN || key == KEY_ENTER;

    if (fw && !usable(fw)) fw = 0;
    if (f->nmenus && (mods() & MOD_CMD) && menu_shortcut(f, key)) return;
    if (key == KEY_TAB) { focus_next(f, (mods() & MOD_SHIFT) ? -1 : 1); return; }
    if (fw && is_edit(fw)) {
        if (is_ret && fw->kind == UI_TEXTFIELD && fw->on_action) { fw->on_action(fw); return; }
        if (edit_key(fw, key)) return;
    } else if (fw && fw->kind == UI_LISTBOX) {
        if (list_key(fw, key)) return;
    }
    if (is_ret && (b = find_flag_button(f, UI_F_DEFAULT)) != 0) { press_button(b); return; }
    if (key == KEY_ESC && (b = find_flag_button(f, UI_F_CANCEL)) != 0) { press_button(b); return; }
    if (f->on_key) f->on_key(f, key);
}

/* ======================================================================== */
/* mouse                                                                    */
/* ======================================================================== */
static int inside(const UIWidget *w, int x, int y)
{
    return x >= w->x && x < w->x + w->w && y >= w->y && y < w->y + w->h;
}

static UIWidget *hit_widget(UIForm *f, int x, int y)
{
    for (int i = f->nw - 1; i >= 0; i--) {
        UIWidget *w = f->wd[i];
        if (!inside(w, x, y) || !visible(w)) continue;
        switch (w->kind) {
        case UI_LABEL: case UI_GROUP: case UI_SEPARATOR: case UI_PROGRESS: continue;
        case UI_TABS:   if (y >= w->y + TAB_H) continue; break;
        case UI_IMAGE:  if (!w->on_action) continue; break;
        case UI_CANVAS: if (!w->on_mouse) continue; break;
        }
        return (w->flags & UI_F_DISABLED) ? 0 : w;
    }
    return 0;
}

/* hold-to-press tracking; returns 1 when released inside */
static int track_press(UIWidget *w)
{
    UIForm *f = w->form;
    int in = 1;
    w->pressed = 1; redraw_now(w);
    while (mouse_btn()) {
        int mx, my, nin;
        mouse_pos(f, &mx, &my);
        nin = inside(w, mx, my);
        if (nin != in) { in = nin; w->pressed = (uint8_t)in; redraw_now(w); }
        nap(1);
    }
    w->pressed = 0; redraw_now(w);
    return in;
}

static void radio_pick(UIWidget *w)
{
    UIForm *f = w->form;
    for (int i = 0; i < f->nw; i++) {
        UIWidget *o = f->wd[i];
        if (o != w && o->kind == UI_RADIO && o->group == w->group && o->value) { o->value = 0; ui_invalidate(o); }
    }
    w->value = 1;
    ui_invalidate(w);
}

static int slider_value(const UIWidget *w, int x, int y)
{
    int range = w->max - w->min, room, off;
    if (range <= 0) return w->min;
    if (w->h > w->w) { room = w->h - 9; off = (w->y + w->h - 5) - y; }
    else             { room = w->w - 9; off = x - (w->x + 4); }
    if (room <= 0) return w->min;
    return w->min + (int)(((long)clampi(off, 0, room) * range + room / 2) / room);
}

static void form_mouse_down(UIForm *f, int sx, int sy)
{
    UIWidget *w;
    int x = sx, y = sy - f->top, dbl, mx, my;
    uint32_t tk = now();

    f->ox = raw_mx() - sx; f->oy = raw_my() - sy;
    if (f->top && sy < f->top) { if (f->nmenus) menu_track(f, sx); return; }
    w = hit_widget(f, x, y);
    if (!w) return;
    dbl = w == f->last_click && tk - f->last_click_tick < DBL_TICKS;
    f->last_click = w; f->last_click_tick = tk;
    if (focusable(w)) ui_focus(w);

    switch (w->kind) {
    case UI_BUTTON: case UI_IMAGE:
        if (track_press(w) && w->on_action) w->on_action(w);
        break;
    case UI_CHECKBOX:
        if (track_press(w)) { w->value = !w->value; ui_invalidate(w); if (w->on_change) w->on_change(w); }
        break;
    case UI_RADIO:
        if (track_press(w) && !w->value) { radio_pick(w); if (w->on_change) w->on_change(w); }
        break;
    case UI_TEXTFIELD: case UI_TEXTAREA:
        caret_show(f);
        w->caret = edit_hit(w, x, y);
        if (!(mods() & MOD_SHIFT)) w->anchor = w->caret;
        if (dbl && w->kind == UI_TEXTFIELD) ui_select_all(w);
        redraw_now(w);
        while (mouse_btn() && !dbl) {                    /* drag to select */
            int p;
            mouse_pos(f, &mx, &my);
            p = edit_hit(w, mx, my);
            if (p != w->caret) { w->caret = p; redraw_now(w); }
            nap(1);
        }
        break;
    case UI_LISTBOX:
        if (list_has_sb(w) && x >= w->x + w->w - SB_W) {
            SB s; list_sb(w, &s);
            sb_track(w, &s, sx, sy, &w->scroll);
            break;
        }
        mx = x; my = y;
        do {
            int row = w->scroll + (clampi(my, w->y + 1, w->y + w->h - 2) - w->y - 1) / LINE_H;
            if (my < w->y && w->scroll > 0) row = w->scroll - 1;
            if (my >= w->y + w->h) row = w->scroll + list_rows(w);
            if (row >= 0 && row < w->nitems && row != w->value) { list_select(w, row); ui_form_draw(f); }
            if (!mouse_btn()) break;
            nap(2);
            mouse_pos(f, &mx, &my);
        } while (1);
        ui_form_draw(f);
        if (dbl && w->on_action && w->value >= 0) w->on_action(w);
        break;
    case UI_COMBO: {
        int r;
        if (!w->nitems) break;
        popup_sticky = 0;
        r = popup_run(f, w->x, w->y + f->top + w->h - 2, w->w - 1, (const char *const *)w->items, w->nitems,
                      w->value, w->x, w->y + f->top, w->x + w->w, w->y + f->top + w->h, 0);
        popup_sticky = 0;
        f->dirty_all = 1;
        if (r >= 0 && r != w->value) { w->value = r; if (w->on_change) w->on_change(w); }
        break;
    }
    case UI_SLIDER:
        mx = x; my = y;
        do {
            int v = slider_value(w, mx, my);
            if (v != w->value) { w->value = v; if (w->on_change) w->on_change(w); redraw_now(w); }
            if (!mouse_btn()) break;
            nap(1);
            mouse_pos(f, &mx, &my);
        } while (1);
        break;
    case UI_SCROLLBAR: {
        SB s; widget_sb(w, &s);
        w->value = s.value;
        sb_track(w, &s, sx, sy, &w->value);
        break;
    }
    case UI_TABS: {
        int t = tab_at(w, x - w->x);
        if (t >= 0 && t != w->value) {
            w->value = t;
            if (f->focus && !visible(f->focus)) f->focus = 0;
            f->dirty_all = 1;
            if (w->on_change) w->on_change(w);
        }
        break;
    }
    case UI_CANVAS:
        w->on_mouse(w, x - w->x, y - w->y, 1);
        ui_form_draw(f);
        while (mouse_btn()) {
            int nx, ny;
            mouse_pos(f, &nx, &ny);
            if (nx != x || ny != y) { x = nx; y = ny; w->on_mouse(w, x - w->x, y - w->y, 1); ui_form_draw(f); }
            nap(1);
        }
        w->on_mouse(w, x - w->x, y - w->y, 0);
        break;
    }
}

/* ======================================================================== */
/* forms                                                                    */
/* ======================================================================== */
UIForm *ui_form_new(const char *title, int w, int h)
{
    UIForm *f = calloc(1, sizeof *f);
    if (!f) return 0;
    strlcpy(f->title, title ? title : "", sizeof f->title);
    f->w = w; f->h = h;
    f->x = f->y = UI_POS_AUTO;
    f->dirty_all = 1;
    f->caret_on = 1;
    return f;
}

static void list_remove_form(UIForm *f)
{
    for (int i = 0; i < nopen; i++)
        if (open_forms[i] == f) {
            for (; i < nopen - 1; i++) open_forms[i] = open_forms[i + 1];
            nopen--;
            return;
        }
}

int ui_form_is_open(const UIForm *f) { return f && f->opened; }

int ui_desktop_running(void)
{
    struct plusos_procinfo p[24];
    struct plusos_proclist_args a = { p, 24 };
    int n = (int)plusos_syscall(SYS_proclist, &a);
    for (int i = 0; i < n; i++)
        if (p[i].state != 4 && !strncmp(p[i].name, "desktop", 7)) return 1;
    return 0;
}

int ui_form_open(UIForm *f)
{
    struct plusos_win_args a;
    int id, i;
    if (!f || f->attached) return -1;
    if (f->opened) return 0;
    if (nopen >= MAX_OPEN || !ui_desktop_running()) return -1;
    memset(&a, 0, sizeof a);
    a.op = PLUSOS_WIN_CREATE; a.title = f->title;
    a.w = f->w; a.h = f->h + f->top;
    a.x = f->x; a.y = f->y;
    if (f->x == UI_POS_CENTER) a.x = imax((plusos->screen_width - (a.w + 16)) / 2, 0);
    if (f->y == UI_POS_CENTER) a.y = imax((plusos->screen_height - (a.h + 26)) / 2, 20);
    if (a.x < 0 || a.y < 0) a.x = a.y = -1;
    id = (int)plusos_syscall(SYS_win, &a);
    if (id <= 0) return -1;
    f->win.id = id;
    gs_wrap(&f->win.surf, a.bits, a.rowbytes, a.w, a.h);
    f->surf = &f->win.surf;
    f->opened = 1;
    open_forms[nopen++] = f;
    if (!f->focus)
        for (i = 0; i < f->nw; i++)
            if (is_edit(f->wd[i]) && usable(f->wd[i])) { f->focus = f->wd[i]; break; }
    caret_show(f);
    f->timer_next = now() + (uint32_t)f->timer_ticks;
    f->dirty_all = 1;
    ui_form_draw(f);
    return 0;
}

void ui_form_close(UIForm *f)
{
    if (!f || !f->opened) return;
    gwin_close(&f->win);
    f->opened = 0;
    f->surf = 0;
    list_remove_form(f);
    if (f->modal) f->modal_done = 1;
}

void ui_form_attach(UIForm *f, GSurface *s)
{
    if (!f || f->opened) return;
    f->surf = s;
    f->attached = s != 0;
    f->dirty_all = 1;
}

static void free_items(UIWidget *w)
{
    for (int i = 0; i < w->nitems; i++) free(w->items[i]);
    free(w->items);
    w->items = 0; w->nitems = w->items_cap = 0;
}

static void free_widget(UIWidget *w)
{
    free_items(w);
    free(w->text);
    free(w);
}

void ui_form_free(UIForm *f)
{
    if (!f) return;
    ui_form_close(f);
    for (int i = 0; i < f->nw; i++) free_widget(f->wd[i]);
    free(f->wd);
    free(f);
}

void ui_form_set_title(UIForm *f, const char *title)
{
    strlcpy(f->title, title ? title : "", sizeof f->title);
    if (f->opened) gwin_title(&f->win, f->title);
}

void ui_form_set_pos(UIForm *f, int x, int y) { f->x = x; f->y = y; }

int ui_form_resize(UIForm *f, int w, int h)
{
    int was = f->opened;
    if (was) ui_form_close(f);
    f->w = w; f->h = h;
    f->dirty_all = 1;
    return was ? ui_form_open(f) : 0;
}

void ui_form_set_menus(UIForm *f, const UIMenu *menus, int n, void (*on_menu)(UIForm *f, int menu, int item))
{
    int was = f->opened, had = f->top;
    f->menus = menus; f->nmenus = menus ? n : 0; f->on_menu = on_menu;
    f->top = f->nmenus ? UI_MENU_H : 0;
    f->dirty_all = 1;
    if (was && had != f->top) { ui_form_close(f); ui_form_open(f); }
}

void ui_form_set_timer(UIForm *f, int ticks, void (*on_timer)(UIForm *f))
{
    f->timer_ticks = ticks; f->on_timer = on_timer;
    f->timer_next = now() + (uint32_t)ticks;
}

void ui_form_end(UIForm *f, int result)
{
    f->result = result;
    f->modal_done = 1;
    if (!f->modal) ui_form_close(f);
}

int ui_form_dispatch(UIForm *f, const GEvent *ev)
{
    if (!f || !f->surf) return 0;
    switch (ev->type) {
    case GEV_MOUSEDOWN:
        form_mouse_down(f, ev->x, ev->y);
        return 1;
    case GEV_KEY:
        form_key(f, ev->key);
        return 1;
    case GEV_CLOSE:
        if (f->on_close && !f->on_close(f)) return 1;
        if (f->modal) { f->result = f->close_result; f->modal_done = 1; }
        else ui_form_close(f);
        return 1;
    }
    return 0;
}

/* blink the caret and fire the timer; returns 0 when the form went away */
static int form_tick(UIForm *f)
{
    uint32_t tk = now();
    if (f->focus && is_edit(f->focus) && (int32_t)(tk - f->blink_next) >= 0) {
        f->caret_on = !f->caret_on;
        f->blink_next = tk + BLINK_TICKS;
        ui_invalidate(f->focus);
    }
    if (f->timer_ticks > 0 && f->on_timer && (int32_t)(tk - f->timer_next) >= 0) {
        f->timer_next = tk + (uint32_t)f->timer_ticks;
        f->on_timer(f);
    }
    return 1;
}

static int form_listed(const UIForm *f)
{
    for (int i = 0; i < nopen; i++) if (open_forms[i] == f) return 1;
    return 0;
}

int ui_step(void)
{
    int got = 0;
    if (quit_flag) return 0;
    for (int i = 0; i < nopen; i++) {
        UIForm *f = open_forms[i];
        GEvent ev;
        int r = gwin_event(&f->win, &ev, 0);
        if (r < 0) {                                    /* the window was destroyed under us */
            f->opened = 0; f->surf = 0;
            list_remove_form(f);
            i--;
            continue;
        }
        if (r > 0) { got = 1; ui_form_dispatch(f, &ev); }
        if (quit_flag) return 0;
        if (!form_listed(f)) { i = -1; continue; }      /* closed or freed by a handler: rescan */
        form_tick(f);
        if (quit_flag) return 0;
        if (!form_listed(f)) { i = -1; continue; }
        ui_form_draw(f);
    }
    if (!nopen) return 0;
    if (!got) nap(1);
    return 1;
}

int ui_run(void)
{
    quit_flag = 0;
    while (ui_step()) ;
    return quit_code;
}

void ui_quit(int code) { quit_code = code; quit_flag = 1; }

int ui_form_run_modal(UIForm *f)
{
    int res;
    if (!f) return UI_CANCEL;
    if (!f->opened && ui_form_open(f) != 0) return f->close_result;
    f->modal = 1; f->modal_done = 0; f->result = f->close_result;
    ui_form_draw(f);
    while (!f->modal_done && f->opened && !quit_flag) {
        GEvent ev;
        int r = gwin_event(&f->win, &ev, 0);
        if (r < 0) { f->opened = 0; f->surf = 0; list_remove_form(f); break; }
        if (r > 0) ui_form_dispatch(f, &ev);
        else {
            int poked = 0;
            for (int i = 0; i < nopen; i++) {           /* the forms behind are inert */
                GEvent e2;
                if (open_forms[i] == f) continue;
                while (gwin_event(&open_forms[i]->win, &e2, 0) > 0)
                    if (e2.type == GEV_MOUSEDOWN) poked = 1;
            }
            /* A click on a form behind means the desktop has raised it over the dialog.
             * There is no "raise" call, so put the dialog back on top by reopening it. */
            if (poked) {
                f->modal = 0;
                ui_form_close(f);
                f->modal = 1;
                if (ui_form_open(f) != 0) break;
                continue;
            }
            nap(1);
        }
        if (f->opened && !f->modal_done) { form_tick(f); ui_form_draw(f); }
    }
    res = f->result;
    f->modal = 0;
    ui_form_close(f);
    return res;
}

/* ======================================================================== */
/* widgets                                                                  */
/* ======================================================================== */
static void set_items(UIWidget *w, const char *list)
{
    char buf[64];
    free_items(w);
    if (!list) return;
    while (*list) {
        int n = 0;
        while (list[n] && list[n] != '|') n++;
        int c = imin(n, (int)sizeof buf - 1);
        memcpy(buf, list, (size_t)c); buf[c] = 0;
        ui_add_item(w, buf);
        list += n;
        if (*list == '|') list++;
    }
}

UIWidget *ui_add(UIForm *f, int kind, int x, int y, int w, int h, const char *text)
{
    UIWidget *wd;
    if (!f) return 0;
    if (f->nw >= f->capw) {
        int nc = f->capw ? f->capw * 2 : 16;
        UIWidget **nv = realloc(f->wd, (size_t)nc * sizeof *nv);
        if (!nv) return 0;
        f->wd = nv; f->capw = nc;
    }
    wd = calloc(1, sizeof *wd);
    if (!wd) return 0;
    wd->form = f; wd->kind = kind;
    wd->x = x; wd->y = y; wd->w = w; wd->h = h;
    wd->max = 100;
    switch (kind) {
    case UI_TEXTFIELD: case UI_TEXTAREA:
        wd->cap = kind == UI_TEXTFIELD ? 63 : 1023;
        wd->text = calloc(1, (size_t)wd->cap + 1);
        if (!wd->text) { free(wd); return 0; }
        f->wd[f->nw++] = wd;
        ui_set_text(wd, text);
        f->dirty_all = 1;
        return wd;
    case UI_LISTBOX: case UI_COMBO: case UI_TABS:
        wd->text = dupstr("");
        wd->value = kind == UI_LISTBOX ? -1 : 0;
        f->wd[f->nw++] = wd;
        set_items(wd, text);
        f->dirty_all = 1;
        return wd;
    case UI_SCROLLBAR:
        wd->page = 10;
        break;
    }
    wd->text = dupstr(text);
    f->wd[f->nw++] = wd;
    f->dirty_all = 1;
    return wd;
}

void ui_remove(UIWidget *w)
{
    UIForm *f;
    if (!w) return;
    f = w->form;
    for (int i = 0; i < f->nw; i++) {
        if (f->wd[i]->tabs == w) f->wd[i]->tabs = 0;
        if (f->wd[i] == w) {
            for (int j = i; j < f->nw - 1; j++) f->wd[j] = f->wd[j + 1];
            f->nw--; i--;
        }
    }
    if (f->focus == w) f->focus = 0;
    if (f->last_click == w) f->last_click = 0;
    f->dirty_all = 1;
    free_widget(w);
}

void ui_set_text(UIWidget *w, const char *text)
{
    if (!w) return;
    if (!text) text = "";
    if (is_edit(w)) {
        strlcpy(w->text, text, (size_t)w->cap + 1);
        w->caret = w->anchor = (int)strlen(w->text);
        w->scroll = 0;
    } else {
        char *d = dupstr(text);
        if (!d) return;
        free(w->text);
        w->text = d;
    }
    ui_invalidate(w);
}

void ui_set_textf(UIWidget *w, const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    ui_set_text(w, buf);
}

const char *ui_get_text(const UIWidget *w) { return w && w->text ? w->text : ""; }

void ui_set_maxlen(UIWidget *w, int maxlen)
{
    char *d;
    if (!w || !is_edit(w) || maxlen < 1) return;
    d = calloc(1, (size_t)maxlen + 1);
    if (!d) return;
    strlcpy(d, w->text, (size_t)maxlen + 1);
    free(w->text);
    w->text = d; w->cap = maxlen;
    w->caret = w->anchor = (int)strlen(d);
    ui_invalidate(w);
}

void ui_select_all(UIWidget *w)
{
    if (!w || !is_edit(w)) return;
    w->anchor = 0; w->caret = (int)strlen(w->text);
    ui_invalidate(w);
}

void ui_set_value(UIWidget *w, int value)
{
    if (!w) return;
    switch (w->kind) {
    case UI_CHECKBOX: w->value = value != 0; break;
    case UI_RADIO:
        if (value) radio_pick(w); else w->value = 0;
        break;
    case UI_SLIDER: case UI_SCROLLBAR: case UI_PROGRESS:
        w->value = clampi(value, w->min, w->max);
        break;
    case UI_LISTBOX: {
        int rows = list_rows(w);
        w->value = clampi(value, -1, w->nitems - 1);
        if (w->value >= 0) {
            if (w->value < w->scroll) w->scroll = w->value;
            if (w->value >= w->scroll + rows) w->scroll = w->value - rows + 1;
        }
        break;
    }
    case UI_COMBO:
        w->value = clampi(value, 0, imax(w->nitems - 1, 0));
        break;
    case UI_TABS:
        w->value = clampi(value, 0, imax(w->nitems - 1, 0));
        w->form->dirty_all = 1;
        break;
    default: w->value = value; break;
    }
    ui_invalidate(w);
}

int ui_get_value(const UIWidget *w) { return w ? w->value : 0; }

void ui_set_range(UIWidget *w, int min, int max, int page)
{
    if (!w) return;
    if (max < min) max = min;
    w->min = min; w->max = max; w->page = page;
    w->value = clampi(w->value, min, max);
    ui_invalidate(w);
}

void ui_set_flags(UIWidget *w, unsigned set, unsigned clear)
{
    unsigned old;
    if (!w) return;
    old = w->flags;
    w->flags = (w->flags | set) & ~clear;
    if ((old ^ w->flags) & UI_F_HIDDEN) w->form->dirty_all = 1;
    if (w->form->focus == w && !usable(w)) w->form->focus = 0;
    ui_invalidate(w);
}

void ui_enable(UIWidget *w, int on) { ui_set_flags(w, on ? 0 : UI_F_DISABLED, on ? UI_F_DISABLED : 0); }
void ui_show(UIWidget *w, int on)   { ui_set_flags(w, on ? 0 : UI_F_HIDDEN, on ? UI_F_HIDDEN : 0); }
void ui_set_group(UIWidget *w, int group) { if (w) w->group = group; }

void ui_set_bounds(UIWidget *w, int x, int y, int width, int height)
{
    if (!w) return;
    w->x = x; w->y = y; w->w = width; w->h = height;
    w->form->dirty_all = 1;
}

void ui_set_image(UIWidget *w, const uint8_t *bits, int bw, int bh)
{
    if (!w) return;
    w->bits = bits; w->bw = bw; w->bh = bh;
    ui_invalidate(w);
}

void ui_set_tab_page(UIWidget *w, UIWidget *tabs, int page)
{
    if (!w || tabs == w) return;
    w->tabs = tabs; w->tab_page = page;
    w->form->dirty_all = 1;
}

int ui_add_item(UIWidget *w, const char *text)
{
    char *d;
    if (!w) return -1;
    if (w->nitems >= w->items_cap) {
        int nc = w->items_cap ? w->items_cap * 2 : 8;
        char **nv = realloc(w->items, (size_t)nc * sizeof *nv);
        if (!nv) return -1;
        w->items = nv; w->items_cap = nc;
    }
    d = dupstr(text);
    if (!d) return -1;
    w->items[w->nitems] = d;
    if (w->kind == UI_TABS) w->form->dirty_all = 1;
    ui_invalidate(w);
    return w->nitems++;
}

void ui_remove_item(UIWidget *w, int index)
{
    if (!w || index < 0 || index >= w->nitems) return;
    free(w->items[index]);
    for (int i = index; i < w->nitems - 1; i++) w->items[i] = w->items[i + 1];
    w->nitems--;
    if (w->value > index) w->value--;
    else if (w->value == index) w->value = w->kind == UI_LISTBOX ? -1 : 0;
    if (w->kind == UI_TABS) w->form->dirty_all = 1;
    ui_invalidate(w);
}

void ui_clear_items(UIWidget *w)
{
    if (!w) return;
    free_items(w);
    w->value = w->kind == UI_LISTBOX ? -1 : 0;
    w->scroll = 0;
    ui_invalidate(w);
}

int ui_item_count(const UIWidget *w) { return w ? w->nitems : 0; }

const char *ui_get_item(const UIWidget *w, int index)
{
    return w && index >= 0 && index < w->nitems ? w->items[index] : "";
}

/* ======================================================================== */
/* dialogs                                                                  */
/* ======================================================================== */
#define DLG_COLS 38

/* word-wrap into a new string; returns the line count and the widest line */
static char *wrap_text(const char *s, int *nlines, int *widest)
{
    size_t n = strlen(s);
    char *d = malloc(n * 2 + 2);
    int o = 0, col = 0, lines = 1, wide = 0;
    if (!d) return 0;
    while (*s) {
        if (*s == '\n') { if (col > wide) wide = col; d[o++] = '\n'; col = 0; lines++; s++; continue; }
        int wl = 0;
        while (s[wl] && s[wl] != ' ' && s[wl] != '\n') wl++;
        if (wl == 0) { if (col && col < DLG_COLS) { d[o++] = ' '; col++; } s++; continue; }
        if (col && col + wl > DLG_COLS) { if (col > wide) wide = col; d[o++] = '\n'; col = 0; lines++; }
        while (wl > 0) {
            if (col >= DLG_COLS) { if (col > wide) wide = col; d[o++] = '\n'; col = 0; lines++; }
            d[o++] = *s++; col++; wl--;
        }
    }
    if (col > wide) wide = col;
    d[o] = 0;
    *nlines = lines; *widest = wide;
    return d;
}

static void dlg_button(UIWidget *w) { ui_form_end(w->form, w->id); }

/* lay a row of buttons out along the bottom, right-aligned; names[0] is rightmost */
static void dlg_buttons(UIForm *f, const char *const *names, const int *ids, int n, int cancel_id)
{
    int x = f->w - 12;
    for (int i = 0; i < n; i++) {
        int bw = i == 0 ? 70 : 64, bh = i == 0 ? 26 : 20;
        UIWidget *b;
        x -= bw;
        b = ui_add(f, UI_BUTTON, x, f->h - 10 - bh - (i == 0 ? 0 : 3), bw, bh, names[i]);
        if (!b) return;
        b->id = ids[i];
        b->on_action = dlg_button;
        if (i == 0) b->flags |= UI_F_DEFAULT;
        if (ids[i] == cancel_id && n > 1) b->flags |= UI_F_CANCEL;
        x -= 8;
    }
}

UIForm *ui_message_form(const char *title, const char *text, int buttons)
{
    static const char *const n_ok[]  = { "OK" };
    static const char *const n_okc[] = { "OK", "Cancel" };
    static const char *const n_yn[]  = { "Yes", "No" };
    static const char *const n_ync[] = { "Yes", "No", "Cancel" };
    static const int i_ok[]  = { UI_OK };
    static const int i_okc[] = { UI_OK, UI_CANCEL };
    static const int i_yn[]  = { UI_YES, UI_NO };
    static const int i_ync[] = { UI_YES, UI_NO, UI_CANCEL };
    const char *const *names = n_ok;
    const int *ids = i_ok;
    int n = 1, cancel = UI_OK, lines = 1, wide = 0, w, h;
    char *wrapped = wrap_text(text ? text : "", &lines, &wide);
    UIForm *f;
    UIWidget *l;

    if (buttons == UI_MB_OKCANCEL)         { names = n_okc; ids = i_okc; n = 2; cancel = UI_CANCEL; }
    else if (buttons == UI_MB_YESNO)       { names = n_yn;  ids = i_yn;  n = 2; cancel = UI_NO; }
    else if (buttons == UI_MB_YESNOCANCEL) { names = n_ync; ids = i_ync; n = 3; cancel = UI_CANCEL; }
    w = imax(wide * 8 + 32, n * 72 + 30);
    w = (imax(w, 176) + 15) & ~15;
    h = lines * LINE_H + 14 + 46;
    f = ui_form_new(title, w, h);
    if (!f) { free(wrapped); return 0; }
    f->x = f->y = UI_POS_CENTER;
    f->close_result = cancel;
    l = ui_add(f, UI_LABEL, 16, 10, w - 32, lines * LINE_H, wrapped ? wrapped : "");
    (void)l;
    free(wrapped);
    dlg_buttons(f, names, ids, n, cancel);
    return f;
}

int ui_message_box(const char *title, const char *text, int buttons)
{
    UIForm *f = ui_message_form(title, text, buttons);
    int r;
    if (!f) return UI_CANCEL;
    r = ui_form_run_modal(f);
    ui_form_free(f);
    return r;
}

int ui_input_box(const char *title, const char *prompt, char *buf, int size)
{
    static const char *const names[] = { "OK", "Cancel" };
    static const int ids[] = { UI_OK, UI_CANCEL };
    UIForm *f;
    UIWidget *e;
    int r;
    if (!buf || size < 2) return 0;
    f = ui_form_new(title, 256, 96);
    if (!f) return 0;
    f->x = f->y = UI_POS_CENTER;
    ui_add(f, UI_LABEL, 16, 8, 224, 12, prompt ? prompt : "");
    e = ui_add(f, UI_TEXTFIELD, 16, 26, 224, 16, "");
    if (!e) { ui_form_free(f); return 0; }
    ui_set_maxlen(e, size - 1);
    ui_set_text(e, buf);
    ui_select_all(e);
    f->focus = e;
    dlg_buttons(f, names, ids, 2, UI_CANCEL);
    r = ui_form_run_modal(f);
    if (r == UI_OK) strlcpy(buf, e->text, (size_t)size);
    ui_form_free(f);
    return r == UI_OK;
}

/* ======================================================================== */
/* clipboard                                                                */
/* ======================================================================== */
void ui_clip_set(const char *text, int len)
{
    struct plusos_scrap_args a;
    memset(&a, 0, sizeof a);
    a.op = PLUSOS_SCRAP_SET; a.type = 0x54455854UL; a.buf = (void *)text; a.size = len;
    plusos_syscall(SYS_scrap, &a);
}

int ui_clip_get(char *buf, int size)
{
    struct plusos_scrap_args a;
    int n;
    memset(&a, 0, sizeof a);
    a.op = PLUSOS_SCRAP_GET; a.buf = buf; a.size = size;
    n = (int)plusos_syscall(SYS_scrap, &a);
    if (n < 0) n = 0;
    if (n >= size) n = size - 1;
    buf[n] = 0;
    return n;
}
