/* snake.c - Retro Snake Game for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <stdlib.h>

#define MAX_SNAKE 100
static int snake_x[MAX_SNAKE];
static int snake_y[MAX_SNAKE];
static int snake_len = 4;
static int dir_x = 1, dir_y = 0;
static int food_x = 15, food_y = 12;
static int score = 0;

int main(int argc, char **argv) {
    for (int i = 0; i < snake_len; i++) {
        snake_x[i] = 10 - i;
        snake_y[i] = 10;
    }

    GSurface *screen = gfx_screen();
    gfx_target(screen);

    bool running = true;
    while (running) {
        if (plusos->key_poll()) {
            char ch = plusos->key_get();
            if (ch == 'q' || ch == 27) break;
            if ((ch == 'a' || ch == 28) && dir_x != 1)  { dir_x = -1; dir_y = 0; }
            if ((ch == 'd' || ch == 29) && dir_x != -1) { dir_x =  1; dir_y = 0; }
            if ((ch == 'w' || ch == 30) && dir_y != 1)  { dir_x =  0; dir_y = -1; }
            if ((ch == 's' || ch == 31) && dir_y != -1) { dir_x =  0; dir_y =  1; }
        }

        /* Move body */
        for (int i = snake_len - 1; i > 0; i--) {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }
        snake_x[0] += dir_x;
        snake_y[0] += dir_y;

        /* Check screen bounds */
        if (snake_x[0] < 1) snake_x[0] = 38;
        if (snake_x[0] > 38) snake_x[0] = 1;
        if (snake_y[0] < 1) snake_y[0] = 24;
        if (snake_y[0] > 24) snake_y[0] = 1;

        /* Check Food */
        if (snake_x[0] == food_x && snake_y[0] == food_y) {
            score += 10;
            if (snake_len < MAX_SNAKE - 1) snake_len++;
            food_x = (rand() % 36) + 2;
            food_y = (rand() % 22) + 2;
        }

        /* Render */
        gfx_fill(0, 0, 512, 342, 0);

        /* Border */
        gfx_frame(10, 30, 502, 330);

        /* Food */
        gfx_fill(10 + food_x * 12, 30 + food_y * 12, 10 + food_x * 12 + 10, 30 + food_y * 12 + 10, 1);

        /* Snake */
        for (int i = 0; i < snake_len; i++) {
            int px = 10 + snake_x[i] * 12;
            int py = 30 + snake_y[i] * 12;
            gfx_fill(px, py, px + 10, py + 10, 1);
        }

        /* HUD */
        gfx_text(16, 12, "PLUSOS SNAKE", 0);
        char s_buf[16];
        itoa(score, s_buf, 10);
        gfx_text(400, 12, "SCORE: ", 0);
        gfx_text(460, 12, s_buf, 0);

        gfx_present();
    }

    plusos->con_clear();
    return 0;
}
