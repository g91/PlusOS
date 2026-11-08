/* invaders.c - Classic Space Invaders Arcade Game for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <stdlib.h>
#include <string.h>

#define ALIEN_ROWS 4
#define ALIEN_COLS 8

static uint8_t aliens[ALIEN_ROWS][ALIEN_COLS];
static int player_x = 256;
static int bullet_x = -1, bullet_y = -1;
static int alien_dir = 1;
static int alien_base_x = 40;
static int alien_base_y = 50;
static int score = 0;

int main(int argc, char **argv) {
    memset(aliens, 1, sizeof(aliens));
    gfx_target(gfx_backbuffer());

    bool running = true;
    while (running) {
        int kp_ = plusos->key_poll();
        if (kp_) {
            char ch = (char)kp_;
            if (ch == 'q' || ch == 27) break;
            if ((ch == 'a' || ch == 28) && player_x > 20) player_x -= 8;
            if ((ch == 'd' || ch == 29) && player_x < 480) player_x += 8;
            if (ch == ' ' && bullet_y < 0) {
                bullet_x = player_x + 8;
                bullet_y = 300;
            }
        }

        /* Move bullet */
        if (bullet_y >= 0) {
            bullet_y -= 8;
            /* Check collision with aliens */
            for (int r = 0; r < ALIEN_ROWS; r++) {
                for (int c = 0; c < ALIEN_COLS; c++) {
                    if (aliens[r][c]) {
                        int ax = alien_base_x + c * 36;
                        int ay = alien_base_y + r * 24;
                        if (bullet_x >= ax && bullet_x <= ax + 20 &&
                            bullet_y >= ay && bullet_y <= ay + 14) {
                            aliens[r][c] = 0;
                            bullet_y = -1;
                            score += 20;
                            break;
                        }
                    }
                }
            }
            if (bullet_y < 20) bullet_y = -1;
        }

        /* Move aliens */
        alien_base_x += alien_dir * 2;
        if (alien_base_x > 180 || alien_base_x < 20) {
            alien_dir *= -1;
            alien_base_y += 8;
        }

        /* Draw */
        gfx_fill(0, 0, 512, 342, 0);

        /* Draw Aliens */
        for (int r = 0; r < ALIEN_ROWS; r++) {
            for (int c = 0; c < ALIEN_COLS; c++) {
                if (aliens[r][c]) {
                    int ax = alien_base_x + c * 36;
                    int ay = alien_base_y + r * 24;
                    gfx_frame(ax, ay, ax + 18, ay + 12);
                    gfx_fill(ax + 4, ay + 3, ax + 14, ay + 9, 1);
                }
            }
        }

        /* Draw Player Cannon */
        gfx_fill(player_x, 310, player_x + 18, 320, 1);
        gfx_fill(player_x + 7, 305, player_x + 11, 310, 1);

        /* Draw Bullet */
        if (bullet_y >= 0) {
            gfx_fill(bullet_x - 1, bullet_y, bullet_x + 1, bullet_y + 6, 1);
        }

        /* Draw Top Bar */
        gfx_line(0, 24, 512, 24, 1);
        gfx_text(16, 8, "SPACE INVADERS", 0);
        gfx_text(380, 8, "SCORE: ", 0);
        char s_buf[16];
        itoa(score, s_buf, 10);
        gfx_text(440, 8, s_buf, 0);

        gfx_present();
    }

    plusos->con_clear();
    return 0;
}
