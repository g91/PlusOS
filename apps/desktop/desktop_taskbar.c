/* desktop_taskbar.c - PlusOS Bottom Taskbar and Start Menu Engine.
 *
 * Implements the interactive Windows 95, Windows 98, Vista/7 Aero, and Hacker Mode
 * bottom taskbars, Start button / Orb / Exec HUD, and popup Start menu.
 */
#include "desktop.h"

void render_taskbar(void)
{
    if (!tb_s.bits && gs_init(&tb_s, SCREEN_W, TASKBAR_H)) return;
    gfx_target(&tb_s);
    gfx_noclip();

    int start_r = 56;
    if (cur_theme == PLUSOS_THEME_HACKER) {
        /* Cyberpunk HUD status / exec bar */
        gfx_fill(0, 0, SCREEN_W, TASKBAR_H, 1);
        gfx_hline(0, SCREEN_W, 0, 0);
        gfx_hline(0, SCREEN_W, 1, 1);
        gfx_hline(0, SCREEN_W, 2, 0);
        gfx_frame(3, 4, 58, TASKBAR_H - 2);
        gfx_text(7, 8, "> EXEC", 1);
        start_r = 62;
    } else if (cur_theme == PLUSOS_THEME_AERO) {
        /* Windows Vista/7 Aero Superbar with circular Start Orb */
        gfx_fill(0, 0, SCREEN_W, TASKBAR_H, 0);
        gfx_dither_rect(0, 2, SCREEN_W, TASKBAR_H, 5);
        gfx_hline(0, SCREEN_W, 0, 1);
        gfx_hline(0, SCREEN_W, 1, 0);
        /* Round Aero Pearl Start Orb */
        gfx_fill_circle(15, 11, 9, 1);
        gfx_circle(15, 11, 10, 1);
        gfx_fill_circle(15, 11, 7, 0);
        /* 4-pane Windows flag inside Orb */
        gfx_fill(11, 7, 15, 11, 1);  gfx_fill(16, 7, 20, 11, 1);
        gfx_fill(11, 12, 15, 16, 1); gfx_fill(16, 12, 20, 16, 1);
        start_r = 32;
    } else {
        /* Windows 95 / 98 3D Taskbar with [Start] button */
        gfx_fill(0, 0, SCREEN_W, TASKBAR_H, 0);
        gfx_hline(0, SCREEN_W, 0, 1);
        gfx_hline(0, SCREEN_W, 1, 0);
        /* 3D raised Start button */
        gfx_frame(2, 3, 56, TASKBAR_H - 2);
        gfx_hline(2, 55, TASKBAR_H - 3, 1);
        gfx_vline(55, 3, TASKBAR_H - 2, 1);
        /* Mini 4-pane Windows logo */
        gfx_fill(6, 7, 10, 11, 1);  gfx_fill(11, 7, 15, 11, 1);
        gfx_fill(6, 12, 10, 16, 1); gfx_fill(11, 12, 15, 16, 1);
        gfx_text(18, 8, "Start", 0);
        start_r = 60;
        if (cur_theme == PLUSOS_THEME_WIN98) {
            /* Win98 Quick Launch divider + icons */
            gfx_vline(60, 4, TASKBAR_H - 3, 1);
            gfx_vline(62, 5, TASKBAR_H - 4, 1);
            gfx_text(66, 8, ">_", 0);
            gfx_vline(86, 4, TASKBAR_H - 3, 1);
            start_r = 90;
        }
    }

    /* Sunken clock tray on the right */
    ULONGINT secs; ReadDateTime(&secs);
    DateTimeRec d; SecondsToDate(secs, &d);
    char clk[10];
    int h = d.hour % 12;
    snprintf(clk, sizeof clk, "%2d:%02d%c", h ? h : 12, d.minute, d.hour < 12 ? 'a' : 'p');
    int tray_w = gfx_text_width(clk) + 14;
    int tray_l = SCREEN_W - tray_w - 3;
    if (cur_theme == PLUSOS_THEME_HACKER) {
        gfx_frame(tray_l, 4, SCREEN_W - 3, TASKBAR_H - 2);
        gfx_text(tray_l + 6, 8, clk, 1);
    } else {
        gfx_fill(tray_l, 4, SCREEN_W - 3, TASKBAR_H - 2, 0);
        gfx_frame(tray_l, 4, SCREEN_W - 3, TASKBAR_H - 2);
        gfx_text(tray_l + 6, 8, clk, 0);
    }

    /* Taskbar buttons for open windows */
    if (nwin > 0) {
        int avail = tray_l - start_r - 4;
        int bw = avail / nwin;
        if (bw > 96) bw = 96;
        if (bw >= 28) {
            for (int i = 0; i < nwin; i++) {
                Window *w = z[i];
                int act = (i == nwin - 1);
                int bl = start_r + i * bw;
                int br = bl + bw - 3;
                char bt[12];
                int maxc = (bw - 10) / 6;
                if (maxc < 2) maxc = 2;
                if (maxc > 10) maxc = 10;
                scopy(bt, w->title, maxc + 1);
                if (cur_theme == PLUSOS_THEME_HACKER) {
                    if (act) {
                        gfx_fill(bl, 4, br, TASKBAR_H - 2, 0);
                        gfx_text(bl + 4, 8, bt, 0);
                    } else {
                        gfx_frame(bl, 4, br, TASKBAR_H - 2);
                        gfx_text(bl + 4, 8, bt, 1);
                    }
                } else if (cur_theme == PLUSOS_THEME_AERO) {
                    gfx_fill(bl, 3, br, TASKBAR_H - 2, act ? 1 : 0);
                    gfx_round_rect(bl, 3, br, TASKBAR_H - 2, 3, 1);
                    gfx_text(bl + 5, 8, bt, act ? 1 : 0);
                } else {
                    if (act) {
                        gfx_dither_rect(bl, 3, br, TASKBAR_H - 2, 3);
                        gfx_frame(bl, 3, br, TASKBAR_H - 2);
                        gfx_hline(bl + 1, br - 1, 4, 1);
                        gfx_vline(bl + 1, 4, TASKBAR_H - 3, 1);
                    } else {
                        gfx_fill(bl, 3, br, TASKBAR_H - 2, 0);
                        gfx_frame(bl, 3, br, TASKBAR_H - 2);
                        gfx_hline(bl, br - 1, TASKBAR_H - 3, 1);
                        gfx_vline(br - 1, 3, TASKBAR_H - 2, 1);
                    }
                    gfx_text(bl + 5, 8, bt, 0);
                }
            }
        }
    }

    gfx_target(BACK);
    tb_dirty = 0;
}

void track_start_menu(void)
{
    static const char *sitems[] = {
        "Programs",
        "Games",
        "Command Shell",
        "File Manager",
        "Calculator",
        "Task Manager",
        "Theme Settings...",
        "Next OS Theme",
        "Shut Down..."
    };
    const int nitems = 9;
    int dl = 2, dr = 164;
    int db = SCREEN_H - TASKBAR_H - 1;
    int dt = db - (nitems * 16 + 6);

    set_overlay(dl, dt, dr + 4, SCREEN_H);
    LOCK();
    gfx_target(BACK); gfx_noclip();
    /* Invert Start button on taskbar while menu is up */
    gfx_invert(2, SCREEN_H - TASKBAR_H + 2, 56, SCREEN_H - 2);
    /* Drop shadow + menu box */
    gfx_fill(dl + 2, dt + 2, dr + 2, db + 2, 1);
    gfx_fill(dl, dt, dr, db, 0);
    gfx_frame(dl, dt, dr, db);
    gfx_frame(dl + 1, dt + 1, dr - 1, db - 1);
    /* Classic vertical OS banner strip on left of Start Menu */
    gfx_fill(dl + 2, dt + 2, dl + 22, db - 2, 1);
    const char *vtag = cur_theme == PLUSOS_THEME_WIN95  ? "WIN95"
                     : cur_theme == PLUSOS_THEME_WIN98  ? "WIN98"
                     : cur_theme == PLUSOS_THEME_AERO   ? "AERO7"
                     :                                    "ROOT!";
    for (int k = 0; vtag[k]; k++) {
        char ch[2] = { vtag[k], 0 };
        gfx_text(dl + 8, db - 18 - (4 - k) * 18, ch, 1);
    }
    for (int i = 0; i < nitems; i++) {
        int y = dt + 3 + i * 16;
        if (i == 6 || i == 8) gfx_hline(dl + 24, dr - 2, y - 1, 1);
        gfx_text(dl + 28, y + 4, sitems[i], 0);
    }
    if (cur_theme == PLUSOS_THEME_HACKER) {
        gfx_invert(dl + 23, dt + 2, dr - 2, db - 2);
    }
    show(dt, SCREEN_H);
    UNLOCK();

    while (mouse_down()) nap(1);    /* allow click-to-open OR drag-to-select */
    int hi = -1, chosen = -1;
    for (;;) {
        int mx = mouse_x(), my = mouse_y();
        int nh = -1;
        if (in_rect(mx, my, dl + 23, dt + 3, dr - 2, db - 3)) {
            nh = (my - dt - 3) / 16;
            if (nh < 0 || nh >= nitems) nh = -1;
        }
        if (nh != hi) {
            LOCK();
            gfx_target(BACK); gfx_noclip();
            if (hi >= 0) gfx_invert(dl + 23, dt + 3 + hi * 16, dr - 2, dt + 3 + hi * 16 + 15);
            if (nh >= 0) gfx_invert(dl + 23, dt + 3 + nh * 16, dr - 2, dt + 3 + nh * 16 + 15);
            show(dt, db);
            UNLOCK();
            hi = nh;
        }
        if (mouse_down()) {
            chosen = hi;
            while (mouse_down()) nap(1);
            break;
        }
        int k = poll_key();
        if (k == KEY_ESC) break;
        nap(1);
    }

    clear_overlay();
    tb_dirty = 1;
    refresh(0, dt, dr + 6, SCREEN_H);

    switch (chosen) {
    case 0: open_programs(); redraw(); break;
    case 1: open_games(); redraw(); break;
    case 2: { char *av[] = { "shell", 0 };   start_prog(1, av); } break;
    case 3: { char *av[] = { "files", 0 };   start_prog(1, av); } break;
    case 4: { char *av[] = { "calc", 0 };    start_prog(1, av); } break;
    case 5: { char *av[] = { "taskman", 0 }; start_prog(1, av); } break;
    case 6: open_settings(); redraw(); break;
    case 7: set_theme((cur_theme + 1) % PLUSOS_THEME_COUNT); break;
    case 8: plusos->shutdown(); break;
    }
}

void click_taskbar(int mx, int my)
{
    (void)my;
    int start_r = (cur_theme == PLUSOS_THEME_AERO) ? 32 : 58;
    if (mx < start_r) {
        track_start_menu();
        return;
    }
    if (cur_theme == PLUSOS_THEME_WIN98 && mx >= 60 && mx < 88) {
        char *av[] = { "shell", 0 };
        start_prog(1, av);
        return;
    }
    int btn_l = (cur_theme == PLUSOS_THEME_WIN98) ? 90 : start_r;
    int tray_l = SCREEN_W - 60;
    if (nwin > 0 && mx >= btn_l && mx < tray_l) {
        int avail = tray_l - btn_l - 4;
        int bw = avail / nwin;
        if (bw > 96) bw = 96;
        if (bw >= 28) {
            int idx = (mx - btn_l) / bw;
            if (idx >= 0 && idx < nwin) {
                Window *w = z[idx];
                Window *old = front();
                bring_front(w);
                focus_desktop = 0;
                if (old && old != w) refresh(old->x, old->y, old->x + old->w + 2, old->y + old->h + 2);
                refresh_win(w);
                refresh(0, SCREEN_H - TASKBAR_H, SCREEN_W, SCREEN_H);
            }
        }
    }
}
