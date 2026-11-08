/* desktop_matrix.c - PlusOS Animated Matrix Digital Rain Wallpaper Engine.
 *
 * Implements the multi-threaded Matrix digital rain animation, coverage
 * exclusion masks, and render suspend/resume handshaking.
 */
#include "desktop.h"

static volatile int ov_on, ov_l, ov_t, ov_r, ov_b;
int cover_dirty = 1;

void set_overlay(int l, int t, int r, int b)
{
    LOCK(); ov_l = l; ov_t = t; ov_r = r; ov_b = b; ov_on = 1; cover_dirty = 1; UNLOCK();
}
void clear_overlay(void)
{
    LOCK(); ov_on = 0; cover_dirty = 1; UNLOCK();
}

static volatile int render_pause_req, render_parked;
static int render_depth;

void render_suspend(void)
{
    if (render_depth++ > 0) return;
    render_pause_req = 1;
    for (int i = 0; i < 200 && !render_parked; i++) nap(1);
}
void render_resume(void)
{
    if (--render_depth <= 0) { render_depth = 0; render_pause_req = 0; }
}

#define MCOLS 64
#define MROWS 40
static int     m_head[MCOLS], m_len[MCOLS];
static uint8_t m_rate[MCOLS], m_cnt[MCOLS];
static uint8_t m_cover[MROWS][MCOLS];
static uint8_t m_pend[MROWS][MCOLS];
static uint32_t rnd_state = 0x1234567;
static const char m_chars[] = "0123456789ABCDEFXZ$#@%&*+=<>?:;!";

static unsigned rnd(void) { rnd_state = rnd_state * 1103515245u + 12345u; return (rnd_state >> 16) & 0x7FFF; }

static void m_reset(int c, int fresh)
{
    m_head[c] = fresh ? -(int)(rnd() % MROWS) : -(int)(rnd() % 24);
    m_len[c] = 6 + (int)(rnd() % 20);
    m_rate[c] = (uint8_t)(1 + rnd() % 3);
    m_cnt[c] = 0;
}

void m_init(void)
{
    if (!wall_s.bits && gs_init(&wall_s, SCREEN_W, SCREEN_H)) { matrix_on = 0; return; }
    memset(wall_s.bits, 0xFF, FB_BYTES);
    memset(m_pend, 0, sizeof m_pend);
    rnd_state ^= now();
    for (int c = 0; c < MCOLS; c++) m_reset(c, 1);
}

void m_cover_update(void)
{
    memset(m_cover, 0, sizeof m_cover);
    for (int i = 0; i < nicons; i++) {
        int c0 = icons[i].x >> 3, c1 = (icons[i].x + ICON_W - 1) >> 3;
        int r0 = (icons[i].y - MENUBAR_H) >> 3, r1 = (icons[i].y + ICON_H - 1 - MENUBAR_H) >> 3;
        for (int r = maxi(r0, 0); r <= r1 && r < MROWS; r++)
            for (int c = maxi(c0, 0); c <= c1 && c < MCOLS; c++) m_cover[r][c] = 1;
    }
    for (int i = 0; i < nwin; i++) {
        Window *w = z[i];
        int c0 = w->x >> 3, c1 = (w->x + w->w + 1) >> 3;
        int r0 = (w->y - MENUBAR_H) >> 3, r1 = (w->y + w->h + 1 - MENUBAR_H) >> 3;
        for (int r = maxi(r0, 0); r <= r1 && r < MROWS; r++)
            for (int c = maxi(c0, 0); c <= c1 && c < MCOLS; c++) m_cover[r][c] = 1;
    }
    if (theme_has_taskbar()) {
        int r0 = (SCREEN_H - TASKBAR_H - MENUBAR_H) >> 3;
        for (int r = maxi(r0, 0); r < MROWS; r++)
            for (int c = 0; c < MCOLS; c++) m_cover[r][c] = 1;
    }
    if (ov_on) {
        int c0 = ov_l >> 3, c1 = (ov_r - 1) >> 3;
        int r0 = (ov_t - MENUBAR_H) >> 3, r1 = (ov_b - 1 - MENUBAR_H) >> 3;
        for (int r = maxi(r0, 0); r <= r1 && r < MROWS; r++)
            for (int c = maxi(c0, 0); c <= c1 && c < MCOLS; c++) m_cover[r][c] = 1;
    }
    cover_dirty = 0;
}

static void m_cell(int c, int r, int style)
{
    uint8_t *p = wall_s.bits + (MENUBAR_H + r * 8) * ROWB + c;
    if (!style) { for (int i = 0; i < 8; i++, p += ROWB) *p = 0xFF; return; }
    int ch = m_chars[rnd() % (sizeof m_chars - 1)];
    const uint8_t *g = plusos->font8x8 + (ch - 0x20) * 8;
    for (int i = 0; i < 8; i++, p += ROWB) *p = style == 2 ? g[i] : (uint8_t)~g[i];
}

static int m_mx0, m_mx1, m_my0, m_my1;

static void m_push(int c, int r)
{
    if (m_cover[r][c]) return;
    if (c >= m_mx0 && c <= m_mx1 && r >= m_my0 && r <= m_my1) { m_pend[r][c] = 1; return; }
    int off = (MENUBAR_H + r * 8) * ROWB + c;
    for (int i = 0; i < 8; i++, off += ROWB) BACK->bits[off] = FRONT->bits[off] = wall_s.bits[off];
    m_pend[r][c] = 0;
}

static void m_step(void)
{
    LOCK();
    if (cover_dirty) m_cover_update();
    int mx = mouse_x(), my = mouse_y();
    m_mx0 = (mx - 6) >> 3; m_mx1 = (mx + 20) >> 3;
    m_my0 = (my - 6 - MENUBAR_H) >> 3; m_my1 = (my + 20 - MENUBAR_H) >> 3;
    for (int r = 0; r < MROWS; r++)
        for (int c = 0; c < MCOLS; c++)
            if (m_pend[r][c]) m_push(c, r);
    UNLOCK();
    for (int c = 0; c < MCOLS; c++) {
        if (++m_cnt[c] < m_rate[c]) continue;
        m_cnt[c] = 0;
        LOCK();
        int h = m_head[c];
        if (h >= 0 && h < MROWS) { m_cell(c, h, 1); m_push(c, h); }
        h = ++m_head[c];
        if (h >= 0 && h < MROWS) { m_cell(c, h, 2); m_push(c, h); }
        int tail = h - m_len[c];
        if (tail >= 0 && tail < MROWS) { m_cell(c, tail, 0); m_push(c, tail); }
        if (h > 2 && (rnd() & 7) == 0) {
            int r = h - 1 - (int)(rnd() % (m_len[c] > 1 ? m_len[c] - 1 : 1));
            if (r >= 0 && r < MROWS && r > tail) { m_cell(c, r, 1); m_push(c, r); }
        }
        if (tail >= MROWS) m_reset(c, 0);
        UNLOCK();
    }
}

void set_wallpaper(int matrix)
{
    render_suspend();
    LOCK();
    if (matrix) { m_init(); matrix_on = wall_s.bits != 0; }
    else { matrix_on = 0; gs_free(&wall_s); }
    UNLOCK();
    render_resume();
    rset("desktop.wallpaper", matrix_on ? "matrix" : "gray");
    redraw();
}

void renderer_thread(void *arg)
{
    (void)arg;
    for (;;) {
        if (render_pause_req) { render_parked = 1; gthread_sleep(2); continue; }
        render_parked = 0;
        if (matrix_on && wall_s.bits) m_step();
        gthread_sleep(2);
    }
}
