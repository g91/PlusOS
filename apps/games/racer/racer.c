/* racer.c - 3D Retro Road Racer for PlusOS
 * Native bare-metal 68000 application running under PlusOS on Macintosh Plus & SE.
 */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <plusrom/ToolboxEvent.h>
#include <plus3d/plus3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACK_SEGMENTS 800
#define ROAD_WIDTH     1800
#define CAM_HEIGHT     800
#define DRAW_DIST      70

typedef struct {
    int y;
    int curve;
} TrackSeg;

static TrackSeg track[TRACK_SEGMENTS];
static float player_x = 0.0f;
static float player_z = 0.0f;
static float speed = 0.0f;
static const float max_speed = 8000.0f;

static void init_track(void) {
    for (int i = 0; i < TRACK_SEGMENTS; i++) {
        track[i].curve = 0;
        track[i].y = 0;
        if (i > 80 && i < 220) track[i].curve = 2;
        else if (i > 280 && i < 420) track[i].curve = -3;
        else if (i > 500 && i < 650) track[i].curve = 3;
    }
}

int main(int argc, char **argv) {
    init_track();
    gfx_target(gfx_backbuffer());

    bool running = true;
    while (running) {
        int kp_ = plusos->key_poll();
        if (kp_) {
            char ch = (char)kp_;
            if (ch == 'q' || ch == 27) break;
            if (ch == 'a' || ch == 28) player_x -= 0.08f; /* Left */
            if (ch == 'd' || ch == 29) player_x += 0.08f; /* Right */
            if (ch == 'w' || ch == 30) speed += 600.0f;   /* Accel */
            if (ch == 's' || ch == 31) speed -= 800.0f;   /* Brake */
        }

        speed *= 0.98f;
        if (speed < 0) speed = 0;
        if (speed > max_speed) speed = max_speed;

        player_z += speed * 0.016f;
        if (player_z >= TRACK_SEGMENTS * 200) player_z = 0;

        /* Render */
        gfx_fill(0, 0, 512, 171, 0); /* Clear sky */
        gfx_gray(0, 171, 512, 342);  /* Gray ground */

        int start_seg = (int)(player_z / 200) % TRACK_SEGMENTS;
        int max_y = 342;
        int curve_acc = 0;

        for (int n = 0; n < DRAW_DIST; n++) {
            int idx = (start_seg + n) % TRACK_SEGMENTS;
            curve_acc += track[idx].curve;

            int z = n * 200 + 10;
            float scale = 300.0f / (float)z;
            int screen_y = 171 + (int)(CAM_HEIGHT * scale);
            int screen_x = 256 + (int)((curve_acc - player_x * ROAD_WIDTH) * scale);
            int road_w   = (int)(ROAD_WIDTH * scale);

            if (screen_y >= max_y) continue;

            int l = screen_x - road_w;
            int r = screen_x + road_w;
            if (l < 0) l = 0;
            if (r > 512) r = 512;

            if (l < r) {
                /* Draw road strip */
                gfx_fill(l, screen_y, r, screen_y + 3, (n % 4 < 2) ? 1 : 0);
            }
            max_y = screen_y;
        }

        /* Draw 3D Player Car */
        int car_x = 256;
        int car_y = 300;
        gfx_frame(car_x - 30, car_y, car_x + 30, car_y + 18);
        gfx_fill(car_x - 18, car_y - 12, car_x + 18, car_y, 1);
        gfx_fill(car_x - 28, car_y + 12, car_x - 18, car_y + 22, 1); /* Wheels */
        gfx_fill(car_x + 18, car_y + 12, car_x + 28, car_y + 22, 1);

        /* HUD */
        gfx_frame(10, 10, 160, 45);
        gfx_text(16, 16, "PLUSOS RETRO RACER", 0);
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", (int)(speed * 0.02f));
        gfx_text(16, 30, "SPEED: ", 0);
        gfx_text(68, 30, buf, 0);
        gfx_text(92, 30, " MPH", 0);

        gfx_present();
    }

    plusos->con_clear();
    return 0;
}
