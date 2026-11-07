/* pacman.c - Classic Maze Chaser Game for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <stdlib.h>

#define MAZE_W 28
#define MAZE_H 20
#define TILE   12

static int pac_x = 14, pac_y = 15;
static int score = 0;
static int ghost_x = 14, ghost_y = 9;

int main(int argc, char **argv) {
    GSurface *screen = gfx_screen();
    gfx_target(screen);

    int off_x = (512 - MAZE_W * TILE) / 2;
    int off_y = 40;

    bool running = true;
    while (running) {
        if (plusos->key_poll()) {
            char ch = plusos->key_get();
            if (ch == 'q' || ch == 27) break;
            if ((ch == 'a' || ch == 28) && pac_x > 1) pac_x--;
            if ((ch == 'd' || ch == 29) && pac_x < MAZE_W - 2) pac_x++;
            if ((ch == 'w' || ch == 30) && pac_y > 1) pac_y--;
            if ((ch == 's' || ch == 31) && pac_y < MAZE_H - 2) pac_y++;
            score += 10;
        }

        /* Simple ghost chase */
        if (rand() % 4 == 0) {
            if (ghost_x < pac_x) ghost_x++;
            else if (ghost_x > pac_x) ghost_x--;
            if (ghost_y < pac_y) ghost_y++;
            else if (ghost_y > pac_y) ghost_y--;
        }

        /* Render */
        gfx_fill(0, 0, 512, 342, 0);

        /* Outer maze border */
        gfx_frame(off_x, off_y, off_x + MAZE_W * TILE, off_y + MAZE_H * TILE);

        /* Draw interior obstacles */
        gfx_fill(off_x + 40, off_y + 30, off_x + 90, off_y + 60, 1);
        gfx_fill(off_x + 120, off_y + 30, off_x + 210, off_y + 60, 1);
        gfx_fill(off_x + 240, off_y + 30, off_x + 290, off_y + 60, 1);

        /* Draw Pacman */
        int px = off_x + pac_x * TILE;
        int py = off_y + pac_y * TILE;
        gfx_fill(px, py, px + 10, py + 10, 1);

        /* Draw Ghost */
        int gx = off_x + ghost_x * TILE;
        int gy = off_y + ghost_y * TILE;
        gfx_frame(gx, gy, gx + 10, gy + 10);
        gfx_pixel(gx + 3, gy + 3, 1);
        gfx_pixel(gx + 7, gy + 3, 1);

        /* HUD */
        gfx_text(16, 12, "PLUSOS PAC-MAN", 0);
        char s_buf[16];
        itoa(score, s_buf, 10);
        gfx_text(400, 12, "SCORE: ", 0);
        gfx_text(460, 12, s_buf, 0);

        gfx_present();
    }

    plusos->con_clear();
    return 0;
}
