/* desktop.c - PlusOS Desktop Compositor, Event Dispatcher, and Main Program.
 *
 * Backing stores, live window dragging, software cursor, compositor flips,
 * and top-level user input handling.
 */
#include "desktop.h"

GSurface *BACK, *FRONT;
GSurface wall_s;
GSurface mb_s;
GSurface tb_s;
int cur_theme = PLUSOS_THEME_MAC;
int tb_dirty = 1;
int mb_dirty = 1;
gmutex_t scr = GMUTEX_INIT;

Icon icons[MAX_ICONS];
int  nicons;
int  icon_sel = -1;
int  focus_desktop = 1;

Window  wins[MAX_WIN];
Window *z[MAX_WIN];
int     nwin;

Menu menus[NMENUS] = {
    { "PlusOS",  { "About PlusOS", "System Info", "Software Center", "Calculator", "TextEdit", "Puzzle", "Task Manager", "Settings..." }, 8 },
    { "File",    { "Open", "Open in Hex Editor", "Programs", "-", "Quit Desktop" }, 5 },
    { "Edit",    { "Copy", "Paste", "Duplicate", "Rename...", "-", "Delete" }, 6 },
    { "View",    { "Clean Up Icons", "-", "Theme Wallpaper", "Matrix Wallpaper" }, 4 },
    { "Theme",   { "Mac OS 7.1", "Commodore Amiga", "Windows 95", "Windows 98", "Windows Vista/7", "Hacker Mode" }, 6 },
    { "Special", { "Start System 7.5.5", "Start System 7.1", "Start System 6.0.8", "-",
                   "Restart", "Shut Down" }, 6 },
};
int menu_x[NMENUS], menu_w[NMENUS];
int matrix_on;
char clip_path[64], clip_name[32];
char bench_str[32];
uint32_t bench_until;
int mb_minute = -1;
INTEGER vref;
int quit_requested;

int prog_sel = -1;
char prog_names[80][32];

/* Compositor */
void compose(int l, int t, int r, int b)
{
    l &= ~7; r = (r + 7) & ~7;
    if (l < 0) l = 0;
    if (t < 0) t = 0;
    if (r > SCREEN_W) r = SCREEN_W;
    if (b > SCREEN_H) b = SCREEN_H;
    if (l >= r || t >= b) return;
    LOCK();
    gfx_target(BACK);
    gfx_clip(l, t, r, b);
    int bt = t < MENUBAR_H ? MENUBAR_H : t;
    int bb = b > desk_bottom() ? desk_bottom() : b;
    if (bt < bb) {
        draw_wallpaper_rect(l, bt, r, bb);
    }
    for (int i = 0; i < nicons; i++) {
        Icon *ic = &icons[i];
        if (!overlap(l, t, r, b, ic->x, ic->y, ic->x + ICON_W, ic->y + ICON_H)) continue;
        int sel = i == icon_sel;
        if (ic->rendered != sel) render_icon(ic, sel);
        gfx_clip(l, t, r, b);
        gfx_blit_masked(&ic->img, &ic->mask, 0, 0, ICON_W, ICON_H, ic->x, ic->y);
    }
    for (int i = 0; i < nwin; i++) {
        Window *w = z[i];
        if (!overlap(l, t, r, b, w->x, w->y, w->x + w->w + 2, w->y + w->h + 2)) continue;
        int active = i == nwin - 1;
        if (w->dirty || w->drawn_active != active || !w->surf.bits) render_window(w, active);
        gfx_clip(l, t, r, b);
        gfx_fill(w->x + 2, w->y + w->h, w->x + w->w + 2, w->y + w->h + 2, 1);
        gfx_fill(w->x + w->w, w->y + 2, w->x + w->w + 2, w->y + w->h, 1);
        gfx_blit(&w->surf, 0, 0, w->w, w->h, w->x, w->y, GFX_COPY);
        if (w->kind == WK_APP && w->app_bits) {
            GSurface s;
            gs_wrap(&s, w->app_bits, w->app_rb, w->app_w, w->app_h);
            gfx_clip(l, t, r, b);
            gfx_blit(&s, 0, 0, w->app_w, w->app_h, w->x + APP_PADX, w->y + APP_TOP, GFX_COPY);
            if (cur_theme == PLUSOS_THEME_HACKER && w->app_term) {
                int il = maxi(l, w->x + APP_PADX), it = maxi(t, w->y + APP_TOP);
                int ir = mini(r, w->x + APP_PADX + w->app_w), ib = mini(b, w->y + APP_TOP + w->app_h);
                if (il < ir && it < ib) gfx_invert(il, it, ir, ib);
            }
        }
    }
    if (theme_has_taskbar() && b > SCREEN_H - TASKBAR_H) {
        if (tb_dirty) render_taskbar();
        gfx_clip(l, t, r, b);
        gfx_blit(&tb_s, 0, 0, SCREEN_W, TASKBAR_H, 0, SCREEN_H - TASKBAR_H, GFX_COPY);
    }
    if (t < MENUBAR_H) {
        if (mb_dirty) render_menubar();
        gfx_clip(l, t, r, b);
        gfx_blit(&mb_s, 0, 0, SCREEN_W, MENUBAR_H, 0, 0, GFX_COPY);
    }
    gfx_noclip();
    cover_dirty = 1;
    UNLOCK();
}

void show(int t, int b)
{
    if (t < 0) t = 0;
    LOCK();
    int my = mouse_y();
    int hide = my + 17 >= t && my - 2 < b;
    if (hide) HideCursor();
    gfx_present_rows(t, b);
    if (hide) ShowCursor();
    UNLOCK();
}

void refresh(int l, int t, int r, int b) { LOCK(); compose(l, t, r, b); show(t, b); UNLOCK(); }
void redraw(void) { refresh(0, 0, SCREEN_W, SCREEN_H); }
void refresh_icon(int i) { if (i >= 0 && i < nicons) refresh(icons[i].x, icons[i].y, icons[i].x + ICON_W, icons[i].y + ICON_H); }
void refresh_win(Window *w) { w->dirty = 1; refresh(w->x, w->y, w->x + w->w + 2, w->y + w->h + 2); }

/* Software cursor */
static GSurface cur_data, cur_mask;
static uint8_t  cur_save[16][3];
static int      cur_x = -100, cur_y = -100;

void swcursor_init(void)
{
    QDGlobals *q = (QDGlobals *)plusos->qd;
    gs_wrap(&cur_data, q->arrow.data, 2, 16, 16);
    gs_wrap(&cur_mask, q->arrow.mask, 2, 16, 16);
}

void present_with_cursor(int t, int b)
{
    QDGlobals *q = (QDGlobals *)plusos->qd;
    LOCK();
    int oy = cur_y;
    cur_x = mouse_x() - q->arrow.hotSpot.h;
    cur_y = mouse_y() - q->arrow.hotSpot.v;
    int x0 = cur_x >> 3;
    for (int r = 0; r < 16; r++) {
        int y = cur_y + r;
        for (int c = 0; c < 3; c++)
            cur_save[r][c] = (y >= 0 && y < SCREEN_H && x0 + c >= 0 && x0 + c < ROWB)
                             ? BACK->bits[y * ROWB + x0 + c] : 0;
    }
    gfx_target(BACK); gfx_noclip();
    gfx_blit_masked(&cur_data, &cur_mask, 0, 0, 16, 16, cur_x, cur_y);
    int pt = mini(t, mini(oy, cur_y)), pb = maxi(b, maxi(oy, cur_y) + 16);
    if (oy == -100) { pt = mini(t, cur_y); pb = maxi(b, cur_y + 16); }
    gfx_present_rows(pt < 0 ? 0 : pt, pb > SCREEN_H ? SCREEN_H : pb);
    for (int r = 0; r < 16; r++) {
        int y = cur_y + r;
        for (int c = 0; c < 3; c++)
            if (y >= 0 && y < SCREEN_H && x0 + c >= 0 && x0 + c < ROWB) BACK->bits[y * ROWB + x0 + c] = cur_save[r][c];
    }
    UNLOCK();
}

void drag_live(int *px, int *py, int w, int h, int gx, int gy, int ymin)
{
    HideCursor();
    cur_x = cur_y = -100;
    present_with_cursor(*py, *py + h);
    while (mouse_down()) {
        int nx = snap16(mouse_x() - gx), ny = mouse_y() - gy;
        if (nx < -w + 32) nx = (-w + 32) & ~15;
        if (nx > SCREEN_W - 32) nx = SCREEN_W - 32;
        if (ny < ymin) ny = ymin;
        if (ny > desk_bottom() - 16) ny = desk_bottom() - 16;
        int moved = nx != *px || ny != *py;
        int cx = mouse_x(), cy = mouse_y();
        if (moved) {
            LOCK();
            int ox = *px, oy = *py;
            *px = nx; *py = ny;
            int l = mini(ox, nx), t = mini(oy, ny), r = maxi(ox, nx) + w, b = maxi(oy, ny) + h;
            compose(l, t, r, b);
            present_with_cursor(t, b);
            UNLOCK();
        } else if (cx - cur_x != 1 || cy - cur_y != 1) {
            present_with_cursor(cy, cy);
        }
        nap(1);
    }
    LOCK();
    compose(cur_x, cur_y, cur_x + 24, cur_y + 16);
    gfx_present_rows(maxi(cur_y, 0), mini(cur_y + 16, SCREEN_H));
    UNLOCK();
    ShowCursor();
}

void drag_window(Window *w, int gx, int gy)
{
    int *px = &w->x, *py = &w->y;
    drag_live(px, py, w->w + 2, w->h + 2, gx, gy, MENUBAR_H);
}

int item_enabled(int m, int i)
{
    Target t;
    if (m == 1 && (i == 0 || i == 1)) {
        int have = get_target(&t);
        return i == 0 ? (have || icon_sel >= 0) : have;
    }
    if (m == 2) {
        int have = get_target(&t);
        if (i == 1) return clip_path[0] != 0;
        if (i == 3 || i == 5) return have && !t.prot;
        return have;
    }
    return 1;
}

int item_checked(int m, int i)
{
    if (m == 3) return (i == 2 && !matrix_on) || (i == 3 && matrix_on);
    if (m == 4) return i == cur_theme;
    return 0;
}

void do_menu(int m, int item)
{
    switch (m) {
    case 0:
        if (item == 0) { open_about(); redraw(); }
        else if (item == 1) { open_info(); redraw(); }
        else if (item == 2) { char *av[] = { "pkgmgr", 0 }; start_prog(1, av); }
        else if (item == 3) { char *av[] = { "calc", 0 }; start_prog(1, av); }
        else if (item == 4) { char *av[] = { "notes", 0 }; start_prog(1, av); }
        else if (item == 5) { char *av[] = { "puzzle", 0 }; start_prog(1, av); }
        else if (item == 6) { char *av[] = { "taskman", 0 }; start_prog(1, av); }
        else if (item == 7) { open_settings(); redraw(); }
        break;
    case 1:
        if (item == 0) cmd_open();
        else if (item == 1) { Target t; if (get_target(&t)) hex_edit(t.path); }
        else if (item == 2) { open_programs(); redraw(); }
        else if (item == 4) quit_requested = 1;
        break;
    case 2:
        if (item == 0) cmd_copy();
        else if (item == 1) cmd_paste();
        else if (item == 2) cmd_duplicate();
        else if (item == 3) cmd_rename();
        else if (item == 5) cmd_delete();
        break;
    case 3:
        if (item == 0) clean_up();
        else if (item == 2) set_wallpaper(0);
        else if (item == 3) set_wallpaper(1);
        break;
    case 4:
        if (item >= 0 && item < PLUSOS_THEME_COUNT) set_theme(item);
        break;
    case 5:
        if (item == 0) start_system(3, "System 7.5.5");
        else if (item == 1) start_system(2, "System 7.1");
        else if (item == 2) start_system(1, "System 6.0.8");
        else if (item == 4) plusos->reboot();
        else if (item == 5) plusos->shutdown();
        break;
    }
}

void track_menus(void)
{
    int open = -1, hi = -1;
    int dl = 0, dt = MENUBAR_H, dr = 0, db = 0;
    for (;;) {
        int mx = mouse_x(), my = mouse_y();
        if (my < MENUBAR_H) {
            int m = -1;
            for (int i = 0; i < NMENUS; i++) if (mx >= menu_x[i] && mx < menu_x[i] + menu_w[i]) m = i;
            if (m != open && m >= 0) {
                LOCK();
                if (open >= 0) compose(0, 0, dr + 4, db + 4);
                int odr = dr, odb = db;
                open = m; hi = -1;
                const Menu *mn = &menus[m];
                int wmax = 0;
                for (int i = 0; i < mn->n; i++) if (gfx_text_width(mn->items[i]) > wmax) wmax = gfx_text_width(mn->items[i]);
                dl = menu_x[m]; dr = dl + wmax + 34; db = dt + mn->n * 16 + 4;
                gfx_target(BACK); gfx_noclip();
                gfx_invert(menu_x[m], 1, menu_x[m] + menu_w[m], MENUBAR_H - 1);
                gfx_fill(dl + 2, dt + 2, dr + 2, db + 2, 1);
                gfx_fill(dl, dt, dr, db, 0);
                gfx_frame(dl, dt, dr, db);
                for (int i = 0; i < mn->n; i++) {
                    int y = dt + 2 + i * 16;
                    if (mn->items[i][0] == '-') { gfx_pattern(dl + 1, y + 8, dr - 1, y + 9, 0xAA, 0xAA); continue; }
                    if (item_checked(m, i)) gfx_text(dl + 4, y + 4, "*", 0);
                    gfx_text(dl + 16, y + 4, mn->items[i], 0);
                    if (!item_enabled(m, i)) gfx_dim(dl + 1, y + 2, dr - 1, y + 14);
                }
                set_overlay(dl, dt, dr + 3, db + 3);
                show(0, maxi(db, odb) + 4);
                UNLOCK();
                (void)odr;
            }
        }
        int nh = -1;
        if (open >= 0 && in_rect(mx, my, dl + 1, dt + 2, dr - 1, db - 2)) {
            nh = (my - dt - 2) / 16;
            if (nh >= menus[open].n || menus[open].items[nh][0] == '-' || !item_enabled(open, nh)) nh = -1;
        }
        if (nh != hi) {
            LOCK();
            gfx_target(BACK); gfx_noclip();
            if (hi >= 0) gfx_invert(dl + 1, dt + 2 + hi * 16, dr - 1, dt + 2 + hi * 16 + 16);
            if (nh >= 0) gfx_invert(dl + 1, dt + 2 + nh * 16, dr - 1, dt + 2 + nh * 16 + 16);
            int a = hi >= 0 ? hi : nh, c = nh >= 0 ? nh : hi;
            show(dt + 2 + mini(a, c) * 16, dt + 2 + maxi(a, c) * 16 + 16);
            UNLOCK();
            hi = nh;
        }
        if (!mouse_down()) break;
        nap(1);
    }
    if (hi >= 0) {
        for (int k = 0; k < 2; k++) {
            LOCK();
            gfx_target(BACK); gfx_noclip();
            gfx_invert(dl + 1, dt + 2 + hi * 16, dr - 1, dt + 2 + hi * 16 + 16);
            show(dt + 2 + hi * 16, dt + 2 + hi * 16 + 16);
            UNLOCK();
            nap(3);
        }
    }
    clear_overlay();
    if (open >= 0) refresh(0, 0, dr + 4, db + 4);
    if (open >= 0 && hi >= 0) do_menu(open, hi);
}

void click(int mx, int my)
{
    if (my < MENUBAR_H) {
        if (cur_theme == PLUSOS_THEME_AMIGA && mx >= SCREEN_W - 24) {
            Window *f = front();
            if (f && nwin > 1) { send_back(f); redraw(); }
            return;
        }
        track_menus();
        return;
    }
    if (theme_has_taskbar() && my >= SCREEN_H - TASKBAR_H) {
        click_taskbar(mx, my);
        return;
    }
    for (int i = nwin - 1; i >= 0; i--) {
        Window *w = z[i];
        if (!in_rect(mx, my, w->x, w->y, w->x + w->w, w->y + w->h)) continue;
        if (w != front()) {
            Window *old = front();
            bring_front(w);
            focus_desktop = 0;
            if (old) refresh(old->x, old->y, old->x + old->w + 2, old->y + old->h + 2);
            refresh(w->x, w->y, w->x + w->w + 2, w->y + w->h + 2);
            if (theme_has_taskbar()) refresh(0, SCREEN_H - TASKBAR_H, SCREEN_W, SCREEN_H);
            if (my < w->y + TITLEBAR_H) drag_window(w, mx - w->x, my - w->y);
            return;
        }
        if (cur_theme == PLUSOS_THEME_AMIGA &&
            in_rect(mx, my, w->x + w->w - 35, w->y + 1, w->x + w->w - 18, w->y + TITLEBAR_H)) {
            if (nwin > 1) { send_back(w); redraw(); }
            return;
        }
        int hit_left_close = in_rect(mx, my, w->x + 4, w->y + 3, w->x + 20, w->y + 15);
        int hit_right_close = (cur_theme != PLUSOS_THEME_MAC) &&
                              in_rect(mx, my, w->x + w->w - 22, w->y + 2, w->x + w->w - 3, w->y + 15);
        if (hit_left_close || hit_right_close) {
            if (w->kind == WK_APP) {
                if (w->app_dead || w->app_term) destroy_app_window(w->app_id);
                else post_ev(w->app_id, PLUSOS_EV_CLOSE, 0, 0, 0);
                return;
            }
            int l = w->x, t = w->y, r = w->x + w->w + 2, b = w->y + w->h + 2;
            if (w->kind == WK_PROGRAMS) { prog_sel = -1; focus_desktop = 1; }
            if (w->kind == WK_GAMES) { game_sel = -1; focus_desktop = 1; }
            close_window(w);
            refresh(l, t, r, b);
            if (theme_has_taskbar()) refresh(0, SCREEN_H - TASKBAR_H, SCREEN_W, SCREEN_H);
            Window *nf = front();
            if (nf) refresh(nf->x, nf->y, nf->x + nf->w + 2, nf->y + nf->h + 2);
            return;
        }
        if (my < w->y + TITLEBAR_H) { drag_window(w, mx - w->x, my - w->y); return; }
        if (w->kind == WK_APP) {
            focus_desktop = 0;
            int cx = mx - w->x - APP_PADX, cy = my - w->y - APP_TOP;
            if (cx >= 0 && cy >= 0 && cx < w->app_w && cy < w->app_h) {
                post_ev(w->app_id, PLUSOS_EV_MOUSEDOWN, cx, cy, 0);
                while (mouse_down()) { sync_apps(); nap(1); }
                post_ev(w->app_id, PLUSOS_EV_MOUSEUP, mouse_x() - w->x - APP_PADX, mouse_y() - w->y - APP_TOP, 0);
            }
            return;
        }
        if (w->click) {
            extern uint32_t last_click_tick;
            extern int last_click_icon;
            uint32_t tk = now();
            int dbl = tk - last_click_tick < 30 && last_click_icon == -2;
            last_click_tick = tk; last_click_icon = -2;
            focus_desktop = 0;
            w->click(w, mx - w->x - 1, my - w->y - TITLEBAR_H, dbl);
            if (dbl) last_click_icon = -1;
        }
        return;
    }
    int i = icon_at(mx, my);
    if (i >= 0) { press_icon(i, mx, my); return; }
    focus_desktop = 1;
    if (icon_sel >= 0) { int old = icon_sel; icon_sel = -1; refresh_icon(old); }
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    FRONT = gfx_screen();
    BACK = gfx_backbuffer();
    vref = (INTEGER)plusos->boot_vrefnum;
    gfx_target(BACK);
    swcursor_init();
    rset("sys.desktop", "1");
    cur_theme = plusos_theme_get();
    layout_menus();
    build_icons();
    char wp[16];
    if ((rget("desktop.wallpaper", wp, sizeof wp) > 0 && !strcmp(wp, "matrix")) ||
        cur_theme == PLUSOS_THEME_HACKER) {
        matrix_on = 1;
        m_init();
    }
    open_about();
    InitCursor();
    redraw();
    gthread_create_ex(renderer_thread, 0, 4096, 1);

    int was_down = 0;
    uint32_t last_sec = now();
    while (!quit_requested) {
        int down = mouse_down();
        if (down && !was_down) click(mouse_x(), mouse_y());
        was_down = mouse_down();

        sync_apps();
        {   struct plusos_reap r;
            do { r.pid = 0; plusos_syscall(SYS_reapany, &r); } while (r.pid > 0); }

        int k = poll_key();
        Window *fw = front();
        if (k && fw && fw->kind == WK_APP && !focus_desktop && !fw->app_dead) {
            post_ev(fw->app_id, PLUSOS_EV_KEY, 0, 0, k);
            k = 0;
        }
        if (k) {
            int cmd = cmd_down();
            if (cmd && (k == 'c' || k == 'C')) cmd_copy();
            else if (cmd && (k == 'v' || k == 'V')) cmd_paste();
            else if (cmd && (k == 'd' || k == 'D')) cmd_duplicate();
            else if (cmd && (k == 'o' || k == 'O')) cmd_open();
            else if (cmd && (k == 'q' || k == 'Q')) quit_requested = 1;
            else if (cmd && k >= '1' && k <= '6') set_theme(k - '1');
            else if (k == KEY_RETURN || k == KEY_ENTER) cmd_open();
            else if (k == KEY_BACKSPACE || k == KEY_DELETE) cmd_delete();
            else if (k == KEY_UP || k == KEY_DOWN || k == KEY_LEFT || k == KEY_RIGHT) {
                int old = icon_sel;
                if (icon_sel < 0) icon_sel = 0;
                else icon_sel = (icon_sel + ((k == KEY_DOWN || k == KEY_RIGHT) ? 1 : nicons - 1)) % nicons;
                focus_desktop = 1;
                if (old >= 0) refresh_icon(old);
                refresh_icon(icon_sel);
            } else if (k == 'p') {
                gfx_target(BACK); gfx_noclip();
                uint32_t t0 = now();
                for (int n = 0; n < 10; n++) {
                    draw_wallpaper_rect(0, MENUBAR_H, SCREEN_W, desk_bottom());
                }
                uint32_t t1 = now();
                for (int n = 0; n < 10; n++)
                    for (int i = 0; i < nicons; i++)
                        gfx_blit_masked(&icons[i].img, &icons[i].mask, 0, 0, ICON_W, ICON_H, icons[i].x, icons[i].y);
                uint32_t t2 = now();
                for (int n = 0; n < 10; n++)
                    for (int i = 0; i < nwin; i++) gfx_blit(&z[i]->surf, 0, 0, z[i]->w, z[i]->h, z[i]->x, z[i]->y, GFX_COPY);
                uint32_t t3 = now();
                for (int n = 0; n < 10; n++) gfx_present_rows(0, SCREEN_H);
                uint32_t t4 = now();
                snprintf(bench_str, sizeof bench_str, "b%lu i%lu w%lu p%lu",
                         (unsigned long)((t1 - t0) * 100 / 60), (unsigned long)((t2 - t1) * 100 / 60),
                         (unsigned long)((t3 - t2) * 100 / 60), (unsigned long)((t4 - t3) * 100 / 60));
                bench_until = now() + 900;
                mb_dirty = 1;
                redraw();
            } else if (k == 'b') {
                uint32_t t0 = now();
                for (int i = 0; i < 5; i++) redraw();
                snprintf(bench_str, sizeof bench_str, "redraw %lums",
                         (unsigned long)((now() - t0) * 1000 / 60 / 5));
                bench_until = now() + 600;
                mb_dirty = 1;
                refresh(0, 0, SCREEN_W, MENUBAR_H);
            }
        }

        uint32_t t = now();
        if (t - last_sec >= 60) {
            last_sec = t;
            int reg_theme = plusos_theme_get();
            if (reg_theme != cur_theme) {
                set_theme(reg_theme);
            }
            ULONGINT secs; ReadDateTime(&secs);
            DateTimeRec d; SecondsToDate(secs, &d);
            if (d.minute != mb_minute || (bench_str[0] && t > bench_until)) {
                if (bench_str[0] && t > bench_until) bench_str[0] = 0;
                mb_dirty = 1;
                tb_dirty = 1;
                refresh(0, 0, SCREEN_W, MENUBAR_H);
                if (theme_has_taskbar()) refresh(0, SCREEN_H - TASKBAR_H, SCREEN_W, SCREEN_H);
            }
            Window *f = front();
            if (f && f->live) refresh_win(f);
        }
        nap(1);
    }
    rset("sys.desktop", "");
    HideCursor();
    con_clear();
    return 0;
}
