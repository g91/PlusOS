/* desktop_theme.c - PlusOS Multi-OS Theme Engine, Window Decoration, and Dialogs.
 *
 * Implements the 6 visual styles:
 *   - Mac OS 7.1
 *   - Commodore Amiga Workbench 1.3/2.0
 *   - Windows 95
 *   - Windows 98
 *   - Windows Vista / 7 Aero Glass
 *   - Hacker Mode (CRT Dark Mode)
 */
#include "desktop.h"

void set_theme(int t)
{
    if (t < 0 || t >= PLUSOS_THEME_COUNT) t = PLUSOS_THEME_MAC;
    render_suspend();
    LOCK();
    int prev = cur_theme;
    cur_theme = t;
    plusos_theme_set(t);
    /* Automatically enable Matrix digital rain when entering Hacker Mode,
     * and return to the theme pattern when leaving Hacker Mode. */
    if (t == PLUSOS_THEME_HACKER && !matrix_on) {
        m_init();
        matrix_on = wall_s.bits != 0;
        rset("desktop.wallpaper", "matrix");
    } else if (prev == PLUSOS_THEME_HACKER && t != PLUSOS_THEME_HACKER && matrix_on) {
        matrix_on = 0;
        gs_free(&wall_s);
        rset("desktop.wallpaper", "gray");
    }
    for (int i = 0; i < nicons; i++) {
        icons[i].rendered = -1;
        if (icons[i].kind == K_TRASH)
            icons[i].y = desk_bottom() - ICON_H - 4;
    }
    for (int i = 0; i < MAX_WIN; i++) {
        if (wins[i].used) wins[i].dirty = 1;
    }
    mb_dirty = 1;
    tb_dirty = 1;
    cover_dirty = 1;
    UNLOCK();
    render_resume();
    redraw();
}

/* Draw a 3D Win95/Win98 caption button at (l, t, r, b) */
void draw_win_btn(int l, int t, int r, int b, int kind)
{
    gfx_fill(l, t, r, b, 0);
    gfx_hline(l, r - 1, t, 0);
    gfx_vline(l, t, b - 1, 0);
    gfx_hline(l, r, b - 1, 1);
    gfx_vline(r - 1, t, b, 1);
    gfx_frame(l, t, r, b);
    int cx = (l + r) / 2, cy = (t + b) / 2;
    if (kind == 0) { /* minimize [_] */
        gfx_hline(cx - 2, cx + 3, b - 3, 1);
        gfx_hline(cx - 2, cx + 3, b - 4, 1);
    } else if (kind == 1) { /* maximize [□] */
        gfx_frame(cx - 3, cy - 3, cx + 3, cy + 3);
        gfx_hline(cx - 3, cx + 3, cy - 2, 1);
    } else { /* close [X] */
        gfx_line(cx - 2, cy - 2, cx + 2, cy + 2, 1);
        gfx_line(cx - 2, cy + 2, cx + 2, cy - 2, 1);
    }
}

void render_window(Window *w, int active)
{
    if (!w->surf.bits && gs_init(&w->surf, w->w, w->h)) return;
    gfx_target(&w->surf);
    gfx_noclip();
    gfx_fill(0, 0, w->w, w->h, 0);
    int tb = TITLEBAR_H - 1;
    int tw = gfx_text_width(w->title);

    switch (cur_theme) {
    case PLUSOS_THEME_MAC:
    default: {
        gfx_frame(0, 0, w->w, w->h);
        gfx_hline(0, w->w, tb, 1);
        if (active) {
            for (int y = 3; y < tb - 2; y += 2) gfx_hline(2, w->w - 2, y, 1);
            gfx_fill(7, 4, 18, 14, 0);
            gfx_frame(7, 4, 18, 14);
            /* System 7 right zoom box */
            gfx_fill(w->w - 18, 4, w->w - 7, 14, 0);
            gfx_frame(w->w - 18, 4, w->w - 7, 14);
            gfx_frame(w->w - 18, 4, w->w - 11, 10);
        }
        int tx = (w->w - tw) / 2;
        if (tx < 24) tx = 24;
        gfx_fill(tx - 5, 2, tx + tw + 5, tb - 1, 0);
        gfx_text(tx, 5, w->title, 0);
        break;
    }
    case PLUSOS_THEME_AMIGA: {
        /* Amiga Workbench 1.3/2.0: double-bordered window, left [|] close gadget,
         * twin thick drag bars, right front/back depth gadgets, bottom-right sizing corner */
        gfx_frame(0, 0, w->w, w->h);
        gfx_frame(1, 1, w->w - 1, w->h - 1);
        gfx_hline(0, w->w, tb, 1);
        gfx_hline(0, w->w, tb - 1, 1);
        /* Close gadget [|] on left */
        gfx_vline(19, 1, tb, 1);
        gfx_frame(4, 3, 16, tb - 2);
        gfx_fill(9, 5, 11, tb - 4, 1);
        /* Title text left-aligned after close gadget */
        int tx = 23;
        gfx_text(tx, 5, w->title, 0);
        /* Twin Amiga drag bars between title and right depth gadgets */
        int drag_l = tx + tw + 6;
        int drag_r = w->w - 38;
        if (drag_l < drag_r) {
            if (active) {
                gfx_fill(drag_l, 4, drag_r, 8, 1);
                gfx_fill(drag_l, 10, drag_r, 14, 1);
            } else {
                gfx_dither_rect(drag_l, 5, drag_r, 8, 8);
                gfx_dither_rect(drag_l, 10, drag_r, 13, 8);
            }
        }
        /* Right depth gadgets (Front / Back overlapping boxes) */
        gfx_vline(w->w - 35, 1, tb, 1);
        gfx_vline(w->w - 18, 1, tb, 1);
        /* Back depth gadget */
        gfx_frame(w->w - 32, 3, w->w - 24, 10);
        gfx_fill(w->w - 29, 6, w->w - 20, 14, 1);
        gfx_fill(w->w - 28, 7, w->w - 21, 13, 0);
        /* Front/Close gadget on far right */
        gfx_fill(w->w - 15, 3, w->w - 7, 11, 1);
        gfx_frame(w->w - 12, 6, w->w - 4, 14);
        /* Amiga bottom-right sizing gadget corner */
        gfx_frame(w->w - 12, w->h - 10, w->w - 1, w->h - 1);
        break;
    }
    case PLUSOS_THEME_WIN95: {
        /* Windows 95: 3D bevel frame, solid dark active title bar, left system icon, right [_][□][X] */
        gfx_frame(0, 0, w->w, w->h);
        gfx_hline(1, w->w - 2, 1, 0);
        gfx_vline(1, 1, w->h - 2, 0);
        gfx_hline(1, w->w - 1, w->h - 2, 1);
        gfx_vline(w->w - 2, 1, w->h - 1, 1);
        gfx_frame(2, 2, w->w - 2, w->h - 2);
        if (active) gfx_fill(3, 3, w->w - 3, tb, 1);
        else        gfx_dither_rect(3, 3, w->w - 3, tb, 8);
        gfx_hline(2, w->w - 2, tb, 1);
        /* Left system menu icon [-] */
        gfx_fill(5, 5, 15, 14, 0);
        gfx_frame(5, 5, 15, 14);
        gfx_hline(7, 13, 9, 1);
        /* Title text */
        if (!active) gfx_fill(18, 4, mini(22 + tw, w->w - 48), tb - 1, 1);
        gfx_text(19, 5, w->title, 1);
        /* Right 3D caption buttons [_][□][X] */
        draw_win_btn(w->w - 42, 4, w->w - 32, 14, 0);
        draw_win_btn(w->w - 30, 4, w->w - 20, 14, 1);
        draw_win_btn(w->w - 17, 4, w->w - 5,  14, 2);
        break;
    }
    case PLUSOS_THEME_WIN98: {
        /* Windows 98: 3D frame + smooth 8-step Bayer dither gradient titlebar (shade 16 -> 4) */
        gfx_frame(0, 0, w->w, w->h);
        gfx_frame(2, 2, w->w - 2, w->h - 2);
        int gl = 3, gr = w->w - 3, gw = gr - gl;
        for (int s = 0; s < 8; s++) {
            int x0 = gl + (gw * s) / 8;
            int x1 = gl + (gw * (s + 1)) / 8;
            int shade = active ? (16 - (s * 12) / 7) : (10 - s);
            gfx_dither_rect(x0, 3, x1, tb, shade);
        }
        gfx_hline(2, w->w - 2, tb, 1);
        /* System icon */
        gfx_fill(5, 5, 15, 14, 0);
        gfx_frame(5, 5, 15, 14);
        gfx_frame(7, 7, 13, 12);
        /* Solid dark backdrop right under title text for crisp legibility on gradient */
        int tbox_r = mini(21 + tw, w->w - 46);
        if (tbox_r > 18) gfx_fill(17, 4, tbox_r, tb - 1, 1);
        gfx_text(19, 5, w->title, 1);
        draw_win_btn(w->w - 42, 4, w->w - 32, 14, 0);
        draw_win_btn(w->w - 30, 4, w->w - 20, 14, 1);
        draw_win_btn(w->w - 17, 4, w->w - 5,  14, 2);
        break;
    }
    case PLUSOS_THEME_AERO: {
        /* Windows Vista / 7 Aero Glass: rounded outer frame, frosted dithered glass titlebar
         * with diagonal specular reflection streaks, rounded title pill, and glass pill buttons */
        gfx_round_rect(0, 0, w->w, w->h, 6, 1);
        gfx_round_rect(1, 1, w->w - 1, w->h - 1, 5, 1);
        gfx_dither_rect(2, 2, w->w - 2, tb, active ? 6 : 3);
        /* Diagonal Aero glass reflection streaks */
        for (int gx = 16; gx < w->w - 40; gx += 44) {
            gfx_line(gx, 2, gx - 10, tb - 1, 0);
            gfx_line(gx + 1, 2, gx - 9, tb - 1, 0);
            gfx_line(gx + 5, 2, gx - 5, tb - 1, 0);
        }
        gfx_hline(2, w->w - 2, tb, 1);
        /* Left Aero orb/close icon */
        gfx_fill_circle(11, 9, 4, 0);
        gfx_circle(11, 9, 4, 1);
        /* Rounded white glass pill behind title */
        int tx = 20;
        gfx_fill_round_rect(tx - 3, 3, mini(tx + tw + 4, w->w - 48), tb - 1, 4, 0);
        gfx_text(tx, 5, w->title, 0);
        /* Right Aero glass pill button group [ - | □ | X ] */
        int pb_l = w->w - 44, pb_r = w->w - 5;
        gfx_fill(pb_l, 2, pb_r, 13, 0);
        gfx_round_rect(pb_l, 2, pb_r, 13, 3, 1);
        gfx_vline(pb_l + 12, 2, 13, 1);
        gfx_vline(pb_l + 24, 2, 13, 1);
        gfx_hline(pb_l + 4, pb_l + 9, 9, 1);
        gfx_frame(pb_l + 15, 5, pb_l + 21, 10);
        if (active) gfx_dither_rect(pb_l + 25, 3, pb_r - 1, 12, 4);
        gfx_line(pb_l + 29, 5, pb_l + 34, 10, 1);
        gfx_line(pb_l + 29, 10, pb_l + 34, 5, 1);
        break;
    }
    case PLUSOS_THEME_HACKER: {
        /* Hacker Mode: cyberpunk bracketed frame, scanline header, [X] kill switch,
         * and inverted dark-mode window interior */
        gfx_fill(0, 0, w->w, w->h, 0);
        gfx_frame(0, 0, w->w, w->h);
        gfx_frame(2, 2, w->w - 2, w->h - 2);
        /* Tech corner bracket ticks */
        gfx_fill(0, 0, 6, 3, 1); gfx_fill(0, 0, 3, 6, 1);
        gfx_fill(w->w - 6, 0, w->w, 3, 1); gfx_fill(w->w - 3, 0, w->w, 6, 1);
        gfx_fill(0, w->h - 3, 6, w->h, 1); gfx_fill(0, w->h - 6, 3, w->h, 1);
        gfx_fill(w->w - 6, w->h - 3, w->w, w->h, 1); gfx_fill(w->w - 3, w->h - 6, w->w, w->h, 1);
        /* Header scanlines */
        gfx_fill(2, 2, w->w - 2, tb, 1);
        if (!active) {
            for (int y = 3; y < tb; y += 2) gfx_hline(3, w->w - 3, y, 0);
        }
        gfx_hline(2, w->w - 2, tb, 0);
        gfx_hline(2, w->w - 2, tb + 1, 1);
        /* Left & right [X] kill switches */
        gfx_text(5, 5, "[X]", 1);
        char htitle[40];
        snprintf(htitle, sizeof htitle, ":: %s ::", w->title);
        int htw = gfx_text_width(htitle);
        int htx = (w->w - htw) / 2;
        if (htx < 30) htx = 30;
        gfx_fill(htx - 2, 4, htx + htw + 2, tb - 1, 1);
        gfx_text(htx, 5, htitle, 1);
        gfx_text(w->w - 22, 5, "[X]", 1);
        break;
    }
    }

    gfx_clip(1, TITLEBAR_H, w->w - 1, w->h - 1);
    if (w->draw) {
        w->draw(w, 1, TITLEBAR_H, w->w - 1, w->h - 1);
        /* In Hacker Mode, invert built-in window client area for true CRT dark mode! */
        if (cur_theme == PLUSOS_THEME_HACKER) {
            gfx_invert(2, TITLEBAR_H + 1, w->w - 2, w->h - 2);
        }
    }
    gfx_target(BACK);
    w->dirty = 0;
    w->drawn_active = active;
}

void layout_menus(void)
{
    int x = 10;
    for (int i = 0; i < NMENUS; i++) { menu_w[i] = gfx_text_width(menus[i].title) + 14; menu_x[i] = x; x += menu_w[i]; }
}

void render_menubar(void)
{
    if (!mb_s.bits && gs_init(&mb_s, SCREEN_W, MENUBAR_H)) return;
    gfx_target(&mb_s);
    gfx_noclip();

    int inv_bar = (cur_theme == PLUSOS_THEME_AMIGA || cur_theme == PLUSOS_THEME_HACKER);
    gfx_fill(0, 0, SCREEN_W, MENUBAR_H, inv_bar ? 1 : 0);
    if (cur_theme == PLUSOS_THEME_AERO) {
        gfx_dither_rect(0, 0, SCREEN_W, MENUBAR_H - 1, 3);
        gfx_hline(0, SCREEN_W, 1, 0);
    }
    gfx_hline(0, SCREEN_W, MENUBAR_H - 1, 1);
    if (cur_theme == PLUSOS_THEME_AMIGA || cur_theme == PLUSOS_THEME_HACKER)
        gfx_hline(0, SCREEN_W, MENUBAR_H - 2, 0);

    /* Classic Mac rounded top screen corners */
    if (cur_theme == PLUSOS_THEME_MAC) {
        gfx_fill(0, 0, 5, 1, 1); gfx_fill(0, 1, 3, 2, 1); gfx_fill(0, 2, 2, 3, 1); gfx_fill(0, 3, 1, 5, 1);
        gfx_fill(SCREEN_W - 5, 0, SCREEN_W, 1, 1); gfx_fill(SCREEN_W - 3, 1, SCREEN_W, 2, 1);
        gfx_fill(SCREEN_W - 2, 2, SCREEN_W, 3, 1); gfx_fill(SCREEN_W - 1, 3, SCREEN_W, 5, 1);
    }

    for (int i = 0; i < NMENUS; i++) {
        if (cur_theme == PLUSOS_THEME_AERO)
            gfx_fill_round_rect(menu_x[i] + 3, 3, menu_x[i] + menu_w[i] - 3, MENUBAR_H - 3, 3, 0);
        gfx_text(menu_x[i] + 7, 6, menus[i].title, inv_bar);
    }

    /* Theme badge in menubar */
    if (bench_str[0]) {
        gfx_text(310, 6, bench_str, inv_bar);
    } else if (cur_theme == PLUSOS_THEME_AMIGA) {
        gfx_text(318, 6, "Workbench", 1);
    } else if (cur_theme == PLUSOS_THEME_HACKER) {
        gfx_text(318, 6, "[ROOT@PLUSOS]", 1);
    }

    ULONGINT secs; ReadDateTime(&secs);
    DateTimeRec d; SecondsToDate(secs, &d);
    char s[8];
    int h = d.hour % 12;
    snprintf(s, sizeof s, "%2d:%02d%c", h ? h : 12, d.minute, d.hour < 12 ? 'a' : 'p');
    int clk_x = SCREEN_W - gfx_text_width(s) - (cur_theme == PLUSOS_THEME_AMIGA ? 28 : 10);
    if (cur_theme == PLUSOS_THEME_AERO)
        gfx_fill(clk_x - 3, 3, clk_x + gfx_text_width(s) + 3, MENUBAR_H - 3, 0);
    gfx_text(clk_x, 6, s, inv_bar);

    /* Amiga Workbench screen depth gadget on far right of menu bar */
    if (cur_theme == PLUSOS_THEME_AMIGA) {
        gfx_vline(SCREEN_W - 22, 0, MENUBAR_H - 1, 0);
        gfx_frame(SCREEN_W - 18, 3, SCREEN_W - 10, 11);
        gfx_fill(SCREEN_W - 14, 7, SCREEN_W - 5, 15, 0);
    }

    mb_minute = d.minute;
    gfx_target(BACK);
    mb_dirty = 0;
}

void draw_wallpaper_rect(int l, int t, int r, int b)
{
    static const uint8_t pat_amiga[8]  = { 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22 };
    static const uint8_t pat_win95[8]  = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };
    static const uint8_t pat_win98[8]  = { 0xCC, 0x66, 0x33, 0x99, 0xCC, 0x66, 0x33, 0x99 };
    static const uint8_t pat_aero[8]   = { 0x82, 0x44, 0x28, 0x10, 0x28, 0x44, 0x82, 0x01 };
    static const uint8_t pat_hacker[8] = { 0xFF, 0xDF, 0xFF, 0xF7, 0xFF, 0xDF, 0xFF, 0x7F };

    if (matrix_on && wall_s.bits) {
        gfx_blit(&wall_s, l, t, r - l, b - t, l, t, GFX_COPY);
        return;
    }
    switch (cur_theme) {
    case PLUSOS_THEME_MAC:
    default:            gfx_gray(l, t, r, b); break;
    case PLUSOS_THEME_AMIGA:  gfx_pattern8(l, t, r, b, pat_amiga);  break;
    case PLUSOS_THEME_WIN95:  gfx_pattern8(l, t, r, b, pat_win95);  break;
    case PLUSOS_THEME_WIN98:  gfx_pattern8(l, t, r, b, pat_win98);  break;
    case PLUSOS_THEME_AERO:   gfx_pattern8(l, t, r, b, pat_aero);   break;
    case PLUSOS_THEME_HACKER: gfx_pattern8(l, t, r, b, pat_hacker); break;
    }
}

/* Dialogs */
void button(int l, int t, int r, int b, const char *label, int dflt)
{
    gfx_fill(l, t, r, b, 0);
    gfx_frame(l, t, r, b);
    if (dflt) gfx_frame(l - 2, t - 2, r + 2, b + 2);
    gfx_text(l + (r - l - gfx_text_width(label)) / 2, t + (b - t - 8) / 2, label, 0);
}

void dlg_frame(int t, int b)
{
    set_overlay(DL, t, DR + 4, b + 4);
    LOCK();
    gfx_target(BACK); gfx_noclip();
    gfx_fill(DL + 3, t + 3, DR + 3, b + 3, 1);
    gfx_fill(DL, t, DR, b, 0);
    gfx_frame(DL, t, DR, b);
    gfx_frame(DL + 3, t + 3, DR - 3, b - 3);
}

int wait_input(int *mx, int *my)
{
    while (mouse_down()) nap(1);
    for (;;) {
        int k = poll_key();
        if (k) return k;
        if (mouse_down()) {
            *mx = mouse_x(); *my = mouse_y();
            while (mouse_down()) nap(1);
            return -1;
        }
        nap(1);
    }
}

void message(const char *l1, const char *l2)
{
    render_suspend();                       /* freeze the animation under the modal dialog */
    int t = 130, b = 196;
    dlg_frame(t, b);
    gfx_text(DL + 16, t + 16, l1, 0);
    if (l2) gfx_text(DL + 16, t + 30, l2, 0);
    button(DR - 80, b - 26, DR - 16, b - 10, "OK", 1);
    show(t, b + 4); UNLOCK();
    int mx = 0, my = 0;
    for (;;) {
        int k = wait_input(&mx, &my);
        if (k > 0 || in_rect(mx, my, DR - 80, b - 26, DR - 16, b - 10)) break;
    }
    clear_overlay(); refresh(DL, t, DR + 4, b + 4); render_resume();
}

int confirm(const char *l1, const char *l2, const char *ok)
{
    render_suspend();
    int t = 130, b = 200;
    dlg_frame(t, b);
    gfx_text(DL + 16, t + 16, l1, 0);
    if (l2) gfx_text(DL + 16, t + 30, l2, 0);
    button(DR - 180, b - 26, DR - 100, b - 10, "Cancel", 0);
    button(DR - 84, b - 26, DR - 16, b - 10, ok, 1);
    show(t, b + 4); UNLOCK();
    int mx = 0, my = 0, res = -1;
    while (res < 0) {
        int k = wait_input(&mx, &my);
        if (k == KEY_RETURN || k == KEY_ENTER) res = 1;
        else if (k == KEY_ESC) res = 0;
        else if (k == -1) {
            if (in_rect(mx, my, DR - 84, b - 26, DR - 16, b - 10)) res = 1;
            else if (in_rect(mx, my, DR - 180, b - 26, DR - 100, b - 10)) res = 0;
        }
    }
    clear_overlay(); refresh(DL, t, DR + 4, b + 4); render_resume();
    return res;
}

int prompt(const char *title, char *buf, int size)
{
    render_suspend();
    int t = 130, b = 206;
    int n = (int)strlen(buf);
    int mx = 0, my = 0, res = -1;
    while (res < 0) {
        dlg_frame(t, b);
        gfx_text(DL + 16, t + 14, title, 0);
        gfx_frame(DL + 16, t + 30, DR - 16, t + 46);
        char vis[40]; int off = n > 35 ? n - 35 : 0;
        scopy(vis, buf + off, sizeof vis);
        int x = gfx_text(DL + 20, t + 34, vis, 0);
        gfx_fill(x, t + 33, x + 2, t + 43, 1);
        button(DR - 180, b - 24, DR - 100, b - 8, "Cancel", 0);
        button(DR - 84, b - 24, DR - 16, b - 8, "OK", 1);
        show(t, b + 4); UNLOCK();
        int k = wait_input(&mx, &my);
        if (k == KEY_RETURN || k == KEY_ENTER) res = 1;
        else if (k == KEY_ESC) res = 0;
        else if (k == KEY_BACKSPACE) { if (n) buf[--n] = 0; }
        else if (k >= 0x20 && k < 0x7F && k != ':' && n < size - 1) { buf[n++] = (char)k; buf[n] = 0; }
        else if (k == -1) {
            if (in_rect(mx, my, DR - 84, b - 24, DR - 16, b - 8)) res = 1;
            else if (in_rect(mx, my, DR - 180, b - 24, DR - 100, b - 8)) res = 0;
        }
    }
    clear_overlay(); refresh(DL, t, DR + 4, b + 4); render_resume();
    return res && buf[0];
}
