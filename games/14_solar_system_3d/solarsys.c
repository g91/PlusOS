/* solarsys.c - 3D Solar System Simulation for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    const char *name;
    float orbit_r;
    float speed;
    int size;
    float angle;
} Planet;

static Planet planets[] = {
    {"MERCURY", 24.0f, 0.08f, 2, 0.0f},
    {"VENUS",   40.0f, 0.05f, 3, 0.0f},
    {"EARTH",   60.0f, 0.035f, 4, 0.0f},
    {"MARS",    80.0f, 0.025f, 3, 0.0f},
    {"JUPITER", 110.0f, 0.015f, 6, 0.0f},
    {"SATURN",  145.0f, 0.010f, 5, 0.0f}
};
#define NUM_PLANETS 6

int main(int argc, char **argv) {
    GSurface *screen = gfx_screen();
    gfx_target(screen);

    float tilt = 0.55f; /* Viewing tilt angle */

    bool running = true;
    while (running) {
        if (plusos->key_poll()) {
            char ch = plusos->key_get();
            if (ch == 'q' || ch == 27) break;
            if (ch == 'w' || ch == 30) tilt = fminf(1.4f, tilt + 0.08f);
            if (ch == 's' || ch == 31) tilt = fmaxf(0.15f, tilt - 0.08f);
        }

        /* Update orbital angles */
        for (int i = 0; i < NUM_PLANETS; i++) {
            planets[i].angle += planets[i].speed;
            if (planets[i].angle > 6.28318f) planets[i].angle -= 6.28318f;
        }

        /* Render */
        gfx_fill(0, 0, 512, 342, 0);

        int cx = 256, cy = 171;
        float cos_t = cosf(tilt);

        /* Draw orbits (ellipses) */
        for (int i = 0; i < NUM_PLANETS; i++) {
            int rx = (int)planets[i].orbit_r;
            int ry = (int)(planets[i].orbit_r * cos_t);
            int prev_x = cx + rx;
            int prev_y = cy;
            for (int s = 1; s <= 32; s++) {
                float a = (float)s * 6.28318f / 32.0f;
                int nx = cx + (int)(cosf(a) * (float)rx);
                int ny = cy + (int)(sinf(a) * (float)ry);
                gfx_line(prev_x, prev_y, nx, ny, 1);
                prev_x = nx;
                prev_y = ny;
            }
        }

        /* Sun */
        gfx_fill(cx - 8, cy - 8, 16, 16, 1);
        gfx_line(cx - 12, cy, cx + 12, cy, 1);
        gfx_line(cx, cy - 12, cx, cy + 12, 1);

        /* Planets */
        for (int i = 0; i < NUM_PLANETS; i++) {
            int px = cx + (int)(cosf(planets[i].angle) * planets[i].orbit_r);
            int py = cy + (int)(sinf(planets[i].angle) * planets[i].orbit_r * cos_t);
            int r = planets[i].size;

            gfx_fill(px - r, py - r, r * 2, r * 2, 1);

            /* Saturn ring */
            if (i == 5) {
                gfx_line(px - r * 2, py, px + r * 2, py, 1);
            }

            gfx_text(px + r + 3, py - 4, (char *)planets[i].name, 1);
        }

        gfx_text(15, 15, "3D SOLAR SYSTEM - PLUSOS", 1);
        gfx_text(15, 30, "W / S: TILT ORBITAL PLANE  Q: QUIT", 1);

        sleep_ticks(2);
    }

    return 0;
}
