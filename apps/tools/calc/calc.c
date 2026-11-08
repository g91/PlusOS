/* calc.c - Classic System 7.1 Calculator Desk Accessory & CLI Evaluator
 *
 * Works in two modes:
 *   1. CLI expression evaluator: `calc 128 * 64 + 512`
 *   2. Interactive GUI Desk Accessory window (when launched from Desktop or
 *      with no arguments).
 */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A desktop window only works when a compositor is running; otherwise opening
 * one and blocking on its events hangs. Detect a live "desktop" process. */
static int under_desktop(void)
{
    struct plusos_procinfo p[16];
    struct plusos_proclist_args a = { p, 16 };
    int n = (int)plusos_syscall(SYS_proclist, &a);
    for (int i = 0; i < n; i++)
        if (p[i].state != 4 && !strncmp(p[i].name, "desktop", 7)) return 1;
    return 0;
}

static const char * const btn_labels[4][4] = {
    { "7", "8", "9", "/" },
    { "4", "5", "6", "*" },
    { "1", "2", "3", "-" },
    { "C", "0", "=", "+" }
};

static long accum = 0;
static long current_val = 0;
static char pending_op = 0;
static int  entering = 0;
static char disp[24] = "0";

static void update_disp(long v)
{
    snprintf(disp, sizeof disp, "%ld", v);
}

static void apply_op(void)
{
    if (!pending_op) {
        accum = current_val;
    } else if (pending_op == '+') {
        accum += current_val;
    } else if (pending_op == '-') {
        accum -= current_val;
    } else if (pending_op == '*' || pending_op == 'x') {
        accum *= current_val;
    } else if (pending_op == '/') {
        if (current_val != 0) accum /= current_val;
        else { strlcpy(disp, "DIV BY 0", sizeof disp); pending_op = 0; entering = 0; return; }
    }
    update_disp(accum);
}

static void press_key(int k)
{
    if (k >= '0' && k <= '9') {
        if (!entering) { current_val = 0; entering = 1; }
        current_val = current_val * 10 + (k - '0');
        update_disp(current_val);
    } else if (k == 'c' || k == 'C') {
        accum = 0; current_val = 0; pending_op = 0; entering = 0;
        strlcpy(disp, "0", sizeof disp);
    } else if (k == '+' || k == '-' || k == '*' || k == '/') {
        if (entering) apply_op();
        pending_op = (char)k;
        entering = 0;
    } else if (k == '=' || k == '\r' || k == 3) {
        if (entering || pending_op) {
            apply_op();
            current_val = accum;
            pending_op = 0;
            entering = 0;
        }
    }
}

static void draw_calc(int w, int h)
{
    gfx_dither_rect(0, 0, w, h, 3);
    gfx_round_rect(4, 4, w - 4, h - 4, 6, 1);

    /* LCD display box */
    gfx_fill_round_rect(12, 12, w - 12, 38, 4, 0);
    gfx_round_rect(12, 12, w - 12, 38, 4, 1);
    int tw = gfx_text_width(disp);
    gfx_text(w - 18 - tw, 21, disp, 0);

    /* 4x4 Keypad */
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int bx = 12 + c * 34;
            int by = 46 + r * 30;
            gfx_fill_round_rect(bx + 2, by + 2, bx + 30, by + 26, 3, 1); /* drop shadow */
            gfx_fill_round_rect(bx, by, bx + 28, by + 24, 3, 0);
            gfx_round_rect(bx, by, bx + 28, by + 24, 3, 1);
            gfx_text(bx + 10, by + 8, btn_labels[r][c], 0);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc >= 2) {
        /* Command-line expression evaluation */
        accum = atol(argv[1]);
        for (int i = 2; i + 1 < argc; i += 2) {
            pending_op = argv[i][0];
            current_val = atol(argv[i + 1]);
            apply_op();
        }
        printf("%s\n", disp);
        return 0;
    }

    GWindow win;
    if (under_desktop() && gwin_open(&win, "Calculator", 156, 174) == 0) {
        gfx_target(&win.surf);
        draw_calc(156, 174);
        gwin_update(&win);
        GEvent ev;
        while (gwin_event(&win, &ev, 1) >= 0) {
            if (ev.type == GEV_CLOSE) break;
            if (ev.type == GEV_KEY) {
                if (ev.key == 'q' || ev.key == 'Q' || ev.key == 27) break;
                press_key(ev.key);
                draw_calc(156, 174);
                gwin_update(&win);
            } else if (ev.type == GEV_MOUSEDOWN) {
                int c = (ev.x - 12) / 34;
                int r = (ev.y - 46) / 30;
                if (r >= 0 && r < 4 && c >= 0 && c < 4) {
                    press_key(btn_labels[r][c][0]);
                    draw_calc(156, 174);
                    gwin_update(&win);
                }
            }
        }
        gwin_close(&win);
        return 0;
    }

    /* Full-screen console fallback */
    printf("PlusOS Calculator (type digits and + - * / =, C to clear, Q to quit)\n");
    for (;;) {
        printf("  [%12s] > ", disp);
        int k = plusos->key_get();
        if (k == 'q' || k == 'Q' || k == 27) { printf("\n"); break; }
        printf("%c\n", k >= 32 && k < 127 ? k : ' ');
        press_key(k);
    }
    return 0;
}
