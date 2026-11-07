/* dungeon.c - 3D Raycasting Dungeon Crawler for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <stdlib.h>
#include <math.h>

#define MAP_W 16
#define MAP_H 11

static const uint8_t MAP[MAP_H][MAP_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,0,1,0,1,1,1,0,1,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,1,0,0,1,0,1},
    {1,0,1,0,1,1,1,1,1,0,1,1,0,1,0,1},
    {1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,1},
    {1,1,1,0,1,0,1,0,1,1,1,1,0,1,1,1},
    {1,0,0,0,1,0,1,0,0,0,0,1,0,0,0,1},
    {1,0,1,1,1,0,1,1,1,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static float player_x = 2.5f, player_y = 2.5f;
static float dir_x = 1.0f, dir_y = 0.0f;
static float plane_x = 0.0f, plane_y = 0.66f;

int main(int argc, char **argv) {
    gfx_target(gfx_backbuffer());

    bool running = true;
    while (running) {
        int kp_ = plusos->key_poll();
        if (kp_) {
            char ch = (char)kp_;
            if (ch == 'q' || ch == 27) break;

            float rot = 0.0f;
            if (ch == 'a' || ch == 28) rot = -0.15f;
            if (ch == 'd' || ch == 29) rot =  0.15f;

            if (rot != 0.0f) {
                float old_dx = dir_x;
                dir_x = dir_x * cosf(rot) - dir_y * sinf(rot);
                dir_y = old_dx * sinf(rot) + dir_y * cosf(rot);
                float old_px = plane_x;
                plane_x = plane_x * cosf(rot) - plane_y * sinf(rot);
                plane_y = old_px * sinf(rot) + plane_y * cosf(rot);
            }

            float move = 0.0f;
            if (ch == 'w' || ch == 30) move =  0.25f;
            if (ch == 's' || ch == 31) move = -0.25f;

            if (move != 0.0f) {
                float nx = player_x + dir_x * move;
                float ny = player_y + dir_y * move;
                if (MAP[(int)player_y][(int)nx] == 0) player_x = nx;
                if (MAP[(int)ny][(int)player_x] == 0) player_y = ny;
            }
        }

        /* Clear screen */
        gfx_fill(0, 0, 512, 342, 0);

        /* Sky horizon & floor stippling */
        for (int y = 171; y < 342; y += 4) {
            for (int x = (y % 8 == 0 ? 0 : 2); x < 512; x += 4) {
                gfx_pixel(x, y, 1);
            }
        }

        /* DDA Raycaster */
        for (int col = 0; col < 256; col++) {
            float camera_x = 2.0f * (float)col / 256.0f - 1.0f;
            float ray_dx = dir_x + plane_x * camera_x;
            float ray_dy = dir_y + plane_y * camera_x;

            int map_x = (int)player_x;
            int map_y = (int)player_y;

            float delta_x = (ray_dx == 0.0f) ? 1e30f : fabsf(1.0f / ray_dx);
            float delta_y = (ray_dy == 0.0f) ? 1e30f : fabsf(1.0f / ray_dy);

            float side_x, side_y;
            int step_x, step_y;

            if (ray_dx < 0.0f) {
                step_x = -1;
                side_x = (player_x - map_x) * delta_x;
            } else {
                step_x = 1;
                side_x = (map_x + 1.0f - player_x) * delta_x;
            }

            if (ray_dy < 0.0f) {
                step_y = -1;
                side_y = (player_y - map_y) * delta_y;
            } else {
                step_y = 1;
                side_y = (map_y + 1.0f - player_y) * delta_y;
            }

            int hit = 0;
            int side = 0;
            while (!hit) {
                if (side_x < side_y) {
                    side_x += delta_x;
                    map_x += step_x;
                    side = 0;
                } else {
                    side_y += delta_y;
                    map_y += step_y;
                    side = 1;
                }
                if (map_x >= 0 && map_x < MAP_W && map_y >= 0 && map_y < MAP_H) {
                    if (MAP[map_y][map_x] > 0) hit = 1;
                } else {
                    hit = 1;
                }
            }

            float perp_dist = (side == 0) ? (side_x - delta_x) : (side_y - delta_y);
            if (perp_dist <= 0.1f) perp_dist = 0.1f;

            int line_h = (int)(342.0f / perp_dist);
            int draw_start = -line_h / 2 + 171;
            if (draw_start < 0) draw_start = 0;
            int draw_end = line_h / 2 + 171;
            if (draw_end >= 342) draw_end = 341;

            int screen_x = col * 2;
            int shade_step = (side == 1) ? 2 : 1;
            if (perp_dist > 5.0f) shade_step = 4;
            else if (perp_dist > 2.5f) shade_step = 2;

            for (int y = draw_start; y <= draw_end; y++) {
                if (shade_step == 1 || ((screen_x + y) % shade_step == 0)) {
                    gfx_pixel(screen_x, y, 1);
                    gfx_pixel(screen_x + 1, y, 1);
                }
            }

            /* Draw wall borders */
            gfx_pixel(screen_x, draw_start, 1);
            gfx_pixel(screen_x + 1, draw_start, 1);
            gfx_pixel(screen_x, draw_end, 1);
            gfx_pixel(screen_x + 1, draw_end, 1);
        }

        /* Draw Minimap in top right */
        int mm_x = 410, mm_y = 10, cell = 5;
        gfx_fill(mm_x - 2, mm_y - 2, mm_x + MAP_W * cell + 2, mm_y + MAP_H * cell + 2, 0);
        gfx_frame(mm_x - 2, mm_y - 2, mm_x + MAP_W * cell + 2, mm_y + MAP_H * cell + 2);
        for (int r = 0; r < MAP_H; r++) {
            for (int c = 0; c < MAP_W; c++) {
                if (MAP[r][c] > 0) {
                    gfx_fill(mm_x + c * cell, mm_y + r * cell, mm_x + (c + 1) * cell, mm_y + (r + 1) * cell, 1);
                }
            }
        }
        /* Draw player on minimap */
        int px = mm_x + (int)(player_x * cell);
        int py = mm_y + (int)(player_y * cell);
        gfx_pixel(px, py, 1);
        gfx_line(px, py, px + (int)(dir_x * 6), py + (int)(dir_y * 6), 1);

        /* HUD Crosshair */
        gfx_line(252, 171, 260, 171, 1);
        gfx_line(256, 167, 256, 175, 1);

        gfx_text(10, 10, "3D DUNGEON CRAWLER - PLUSOS", 0);
        gfx_text(10, 24, "WASD / ARROWS: MOVE  Q: QUIT", 0);

        gfx_present();
        sleep_ticks(1);
    }

    return 0;
}
