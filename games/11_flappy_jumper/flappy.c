/* flappy.c - Flappy Jumper for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_PIPES 3

typedef struct {
    int x;
    int gap_y;
    int gap_h;
    bool passed;
} Pipe;

static float bird_y = 150.0f;
static float bird_vy = 0.0f;
static Pipe pipes[NUM_PIPES];
static int score = 0;
static bool game_over = false;

static void reset_game(void) {
    bird_y = 150.0f;
    bird_vy = 0.0f;
    score = 0;
    game_over = false;
    for (int i = 0; i < NUM_PIPES; i++) {
        pipes[i].x = 300 + i * 160;
        pipes[i].gap_y = 70 + (rand() % 140);
        pipes[i].gap_h = 75;
        pipes[i].passed = false;
    }
}

int main(int argc, char **argv) {
    GSurface *screen = gfx_screen();
    gfx_target(screen);

    reset_game();

    bool running = true;
    while (running) {
        if (plusos->key_poll()) {
            char ch = plusos->key_get();
            if (ch == 'q' || ch == 27) break;
            if (ch == ' ' || ch == 'w' || ch == 30) {
                if (game_over) reset_game();
                else bird_vy = -6.5f;
            }
        }

        if (!game_over) {
            bird_vy += 0.45f; /* Gravity */
            bird_y += bird_vy;

            if (bird_y < 10.0f) {
                bird_y = 10.0f;
                bird_vy = 0;
            }
            if (bird_y > 300.0f) {
                bird_y = 300.0f;
                game_over = true;
            }

            for (int i = 0; i < NUM_PIPES; i++) {
                pipes[i].x -= 3;
                if (!pipes[i].passed && pipes[i].x < 80) {
                    pipes[i].passed = true;
                    score++;
                }

                /* Collision check */
                if (pipes[i].x < 88 && pipes[i].x + 36 > 72) {
                    if (bird_y - 6 < pipes[i].gap_y || bird_y + 6 > pipes[i].gap_y + pipes[i].gap_h) {
                        game_over = true;
                    }
                }

                if (pipes[i].x < -40) {
                    pipes[i].x = pipes[(i + NUM_PIPES - 1) % NUM_PIPES].x + 160;
                    pipes[i].gap_y = 70 + (rand() % 140);
                    pipes[i].passed = false;
                }
            }
        }

        /* Render */
        gfx_fill(0, 0, 512, 342, 0);

        /* Pipes */
        for (int i = 0; i < NUM_PIPES; i++) {
            int px = pipes[i].x;
            if (px > -40 && px < 512) {
                /* Upper pipe */
                gfx_fill(px, 0, 36, pipes[i].gap_y, 1);
                gfx_frame(px - 2, pipes[i].gap_y - 12, px + 38, pipes[i].gap_y);
                /* Lower pipe */
                int by = pipes[i].gap_y + pipes[i].gap_h;
                gfx_fill(px, by, 36, 310 - by, 1);
                gfx_frame(px - 2, by, px + 38, by + 12);
            }
        }

        /* Ground line */
        gfx_line(0, 310, 511, 310, 1);
        for (int gx = 0; gx < 512; gx += 16) {
            gfx_line(gx, 310, gx + 8, 341, 1);
        }

        /* Bird */
        int by = (int)bird_y;
        gfx_frame(72, by - 6, 88, by + 6);
        gfx_pixel(84, by - 2, 1); /* Eye */
        gfx_line(88, by, 94, by + 1, 1); /* Beak */

        /* HUD */
        char buf[32];
        snprintf(buf, sizeof(buf), "SCORE: %d", score);
        gfx_text(220, 15, buf, 1);

        if (game_over) {
            gfx_frame(170, 130, 342, 200);
            gfx_fill(172, 132, 168, 66, 0);
            gfx_text(205, 145, "GAME OVER!", 1);
            gfx_text(185, 170, "PRESS SPACE TO RETRY", 1);
        }

        sleep_ticks(2);
    }

    return 0;
}
