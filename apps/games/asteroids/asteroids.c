/* asteroids.c - Vector Space Rock Shooter for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265f
#endif

typedef struct {
    float x, y, vx, vy;
    int size;
    bool active;
} Asteroid;

#define MAX_AST 12
static Asteroid asts[MAX_AST];
static float ship_x = 256.0f, ship_y = 171.0f;
static float ship_vx = 0.0f, ship_vy = 0.0f;
static float ship_angle = 0.0f;
static int score = 0;

int main(int argc, char **argv) {
    for (int i = 0; i < 4; i++) {
        asts[i].active = true;
        asts[i].x = (float)(rand() % 512);
        asts[i].y = (float)(rand() % 342);
        asts[i].vx = (float)((rand() % 5) - 2);
        asts[i].vy = (float)((rand() % 5) - 2);
        asts[i].size = 20;
    }

    gfx_target(gfx_backbuffer());

    bool running = true;
    while (running) {
        int kp_ = plusos->key_poll();
        if (kp_) {
            char ch = (char)kp_;
            if (ch == 'q' || ch == 27) break;
            if (ch == 'a' || ch == 28) ship_angle -= 0.2f;
            if (ch == 'd' || ch == 29) ship_angle += 0.2f;
            if (ch == 'w' || ch == 30) {
                ship_vx += cosf(ship_angle) * 1.5f;
                ship_vy += sinf(ship_angle) * 1.5f;
            }
        }

        ship_vx *= 0.98f;
        ship_vy *= 0.98f;
        ship_x += ship_vx;
        ship_y += ship_vy;

        if (ship_x < 0) ship_x += 512;
        if (ship_x >= 512) ship_x -= 512;
        if (ship_y < 0) ship_y += 342;
        if (ship_y >= 342) ship_y -= 342;

        /* Update Asteroids */
        for (int i = 0; i < MAX_AST; i++) {
            if (asts[i].active) {
                asts[i].x += asts[i].vx;
                asts[i].y += asts[i].vy;
                if (asts[i].x < 0) asts[i].x += 512;
                if (asts[i].x >= 512) asts[i].x -= 512;
                if (asts[i].y < 0) asts[i].y += 342;
                if (asts[i].y >= 342) asts[i].y -= 342;
            }
        }

        /* Render */
        gfx_fill(0, 0, 512, 342, 0);

        /* Draw Asteroids */
        for (int i = 0; i < MAX_AST; i++) {
            if (asts[i].active) {
                int ax = (int)asts[i].x;
                int ay = (int)asts[i].y;
                int r = asts[i].size;
                gfx_frame(ax - r, ay - r, ax + r, ay + r);
            }
        }

        /* Draw Ship (triangle) */
        int x0 = (int)(ship_x + cosf(ship_angle) * 12);
        int y0 = (int)(ship_y + sinf(ship_angle) * 12);
        int x1 = (int)(ship_x + cosf(ship_angle + 2.5f) * 10);
        int y1 = (int)(ship_y + sinf(ship_angle + 2.5f) * 10);
        int x2 = (int)(ship_x + cosf(ship_angle - 2.5f) * 10);
        int y2 = (int)(ship_y + sinf(ship_angle - 2.5f) * 10);

        gfx_line(x0, y0, x1, y1, 1);
        gfx_line(x1, y1, x2, y2, 1);
        gfx_line(x2, y2, x0, y0, 1);

        /* HUD */
        gfx_text(16, 12, "PLUSOS ASTEROIDS", 0);
        gfx_present();
    }

    plusos->con_clear();
    return 0;
}
