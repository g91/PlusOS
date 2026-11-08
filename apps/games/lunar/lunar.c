/* lunar.c - Lunar Lander Physics Simulation for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <stdlib.h>
#include <math.h>

static float lander_x = 100.0f, lander_y = 60.0f;
static float vx = 1.0f, vy = 0.0f;
static float fuel = 500.0f;
static bool thrusting = false;

int main(int argc, char **argv) {
    gfx_target(gfx_backbuffer());

    bool running = true;
    while (running) {
        thrusting = false;
        int kp_ = plusos->key_poll();
        if (kp_) {
            char ch = (char)kp_;
            if (ch == 'q' || ch == 27) break;
            if ((ch == 'a' || ch == 28) && fuel > 0) { vx -= 0.3f; fuel -= 2.0f; }
            if ((ch == 'd' || ch == 29) && fuel > 0) { vx += 0.3f; fuel -= 2.0f; }
            if ((ch == 'w' || ch == 30 || ch == ' ') && fuel > 0) {
                vy -= 0.6f;
                fuel -= 4.0f;
                thrusting = true;
            }
        }

        vy += 0.15f; /* Gravity */
        lander_x += vx;
        lander_y += vy;

        if (lander_y > 290.0f) {
            lander_y = 290.0f;
            vy = 0.0f;
            vx = 0.0f;
        }

        /* Render */
        gfx_fill(0, 0, 512, 342, 0);

        /* Moon Surface */
        gfx_line(0, 300, 180, 290, 1);
        gfx_line(180, 290, 240, 290, 1); /* Pad */
        gfx_line(240, 290, 512, 310, 1);

        /* Draw Lander */
        int lx = (int)lander_x;
        int ly = (int)lander_y;
        gfx_frame(lx - 8, ly - 8, lx + 8, ly + 4);
        gfx_line(lx - 6, ly + 4, lx - 10, ly + 10, 1);
        gfx_line(lx + 6, ly + 4, lx + 10, ly + 10, 1);

        if (thrusting) {
            gfx_line(lx - 3, ly + 5, lx, ly + 12, 1);
            gfx_line(lx + 3, ly + 5, lx, ly + 12, 1);
        }

        /* HUD */
        gfx_text(16, 12, "PLUSOS LUNAR LANDER", 0);
        char f_buf[16];
        itoa((int)fuel, f_buf, 10);
        gfx_text(380, 12, "FUEL: ", 0);
        gfx_text(430, 12, f_buf, 0);

        gfx_present();
    }

    plusos->con_clear();
    return 0;
}
