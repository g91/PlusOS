/* tetris.c - Classic Falling Polyomino Block Game for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <stdlib.h>
#include <string.h>

#define WELL_W 10
#define WELL_H 20
#define BLOCK_SZ 12

static uint8_t well[WELL_H][WELL_W];
static int score = 0;
static int lines = 0;
static int level = 1;

int main(int argc, char **argv) {
    gfx_target(gfx_backbuffer());
    gfx_fill(0, 0, 512, 342, 0);

    int well_x = (512 - WELL_W * BLOCK_SZ) / 2;
    int well_y = 40;

    int cur_x = 4, cur_y = 0;
    int piece_timer = 0;
    bool running = true;

    while (running) {
        int kp_ = plusos->key_poll();
        if (kp_) {
            char ch = (char)kp_;
            if (ch == 'q' || ch == 27) break;
            if (ch == 'a' || ch == 28) { if (cur_x > 0) cur_x--; }
            if (ch == 'd' || ch == 29) { if (cur_x < WELL_W - 2) cur_x++; }
            if (ch == 's' || ch == 31) { cur_y++; }
            if (ch == ' ') { while (cur_y < WELL_H - 2) cur_y++; }
        }

        piece_timer++;
        if (piece_timer > 20) {
            piece_timer = 0;
            cur_y++;
            if (cur_y >= WELL_H - 1) {
                well[cur_y][cur_x] = 1;
                well[cur_y][cur_x + 1] = 1;
                cur_x = 4;
                cur_y = 0;
                score += 10;
            }
        }

        /* Clear display */
        gfx_fill(0, 0, 512, 342, 0);

        /* Draw Well */
        gfx_frame(well_x - 2, well_y - 2, well_x + WELL_W * BLOCK_SZ + 2, well_y + WELL_H * BLOCK_SZ + 2);
        for (int y = 0; y < WELL_H; y++) {
            for (int x = 0; x < WELL_W; x++) {
                if (well[y][x]) {
                    int bx = well_x + x * BLOCK_SZ;
                    int by = well_y + y * BLOCK_SZ;
                    gfx_fill(bx + 1, by + 1, bx + BLOCK_SZ - 1, by + BLOCK_SZ - 1, 1);
                }
            }
        }

        /* Draw current active piece */
        int px = well_x + cur_x * BLOCK_SZ;
        int py = well_y + cur_y * BLOCK_SZ;
        gfx_fill(px + 1, py + 1, px + BLOCK_SZ * 2 - 1, py + BLOCK_SZ - 1, 1);

        /* Draw UI */
        gfx_frame(20, 40, 140, 120);
        gfx_text(26, 48, "PLUSOS TETRIS", 0);
        gfx_text(26, 70, "SCORE:", 0);
        char s_buf[16];
        itoa(score, s_buf, 10);
        gfx_text(75, 70, s_buf, 0);

        gfx_text(26, 90, "LINES:", 0);
        itoa(lines, s_buf, 10);
        gfx_text(75, 90, s_buf, 0);

        gfx_present();
    }

    plusos->con_clear();
    return 0;
}
