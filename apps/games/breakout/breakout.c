/* breakout.c - Paddle & Brick Breaker for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <stdio.h>
#include <stdlib.h>

#define ROWS 4
#define COLS 10
static uint8_t bricks[ROWS][COLS];
static int paddle_x = 220;
static int ball_x = 256, ball_y = 200;
static int ball_dx = 3, ball_dy = -3;
static int score = 0;

int main(int argc, char **argv) {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            bricks[r][c] = 1;

    gfx_target(gfx_backbuffer());

    bool running = true;
    while (running) {
        int kp_ = plusos->key_poll();
        if (kp_) {
            char ch = (char)kp_;
            if (ch == 'q' || ch == 27) break;
            if ((ch == 'a' || ch == 28) && paddle_x > 20) paddle_x -= 12;
            if ((ch == 'd' || ch == 29) && paddle_x < 420) paddle_x += 12;
        }

        /* Move Ball */
        ball_x += ball_dx;
        ball_y += ball_dy;

        if (ball_x <= 15 || ball_x >= 495) ball_dx = -ball_dx;
        if (ball_y <= 35) ball_dy = -ball_dy;

        /* Paddle Hit */
        if (ball_y >= 310 && ball_y <= 316 && ball_x >= paddle_x && ball_x <= paddle_x + 60) {
            ball_dy = -abs(ball_dy);
        }

        /* Bottom Drain */
        if (ball_y > 330) {
            ball_x = 256;
            ball_y = 200;
            ball_dy = -3;
        }

        /* Brick Hit */
        int off_x = 45, off_y = 50;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (bricks[r][c]) {
                    int bx = off_x + c * 42;
                    int by = off_y + r * 16;
                    if (ball_x >= bx && ball_x <= bx + 38 && ball_y >= by && ball_y <= by + 12) {
                        bricks[r][c] = 0;
                        ball_dy = -ball_dy;
                        score += 20;
                    }
                }
            }
        }

        /* Render */
        gfx_fill(0, 0, 512, 342, 0);

        /* Borders */
        gfx_line(10, 30, 502, 30, 1);
        gfx_line(10, 30, 10, 340, 1);
        gfx_line(502, 30, 502, 340, 1);

        /* Draw Bricks */
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (bricks[r][c]) {
                    int bx = off_x + c * 42;
                    int by = off_y + r * 16;
                    gfx_frame(bx, by, bx + 38, by + 12);
                    gfx_fill(bx + 2, by + 2, bx + 36, by + 10, 1);
                }
            }
        }

        /* Paddle */
        gfx_fill(paddle_x, 316, paddle_x + 60, 324, 1);

        /* Ball */
        gfx_fill(ball_x - 3, ball_y - 3, ball_x + 3, ball_y + 3, 1);

        /* HUD */
        gfx_text(16, 12, "PLUSOS BREAKOUT", 0);
        char s_buf[16];
        snprintf(s_buf, sizeof(s_buf), "%d", score);
        gfx_text(400, 12, "SCORE: ", 0);
        gfx_text(460, 12, s_buf, 0);

        gfx_present();
    }

    plusos->con_clear();
    return 0;
}
