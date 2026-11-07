/* mines.c - Classic Minesweeper for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <stdlib.h>

#define COLS 16
#define ROWS 12
#define CELL 16
#define MINES 24

static int8_t grid[ROWS][COLS];
static uint8_t state[ROWS][COLS]; /* 0: hidden, 1: revealed, 2: flagged */
static int cur_x = 0, cur_y = 0;
static bool initialized = false;
static bool game_over = false;
static bool game_won = false;

static void place_mines(int safe_x, int safe_y) {
    int placed = 0;
    while (placed < MINES) {
        int r = rand() % ROWS;
        int c = rand() % COLS;
        if (r == safe_y && c == safe_x) continue;
        if (grid[r][c] != -1) {
            grid[r][c] = -1;
            placed++;
        }
    }

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (grid[r][c] == -1) continue;
            int count = 0;
            for (int dr = -1; dr <= 1; dr++) {
                for (int dc = -1; dc <= 1; dc++) {
                    int nr = r + dr, nc = c + dc;
                    if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS) {
                        if (grid[nr][nc] == -1) count++;
                    }
                }
            }
            grid[r][c] = count;
        }
    }
}

static void reveal(int r, int c) {
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS) return;
    if (state[r][c] != 0) return;

    if (!initialized) {
        place_mines(c, r);
        initialized = true;
    }

    state[r][c] = 1;
    if (grid[r][c] == -1) {
        game_over = true;
        return;
    }

    if (grid[r][c] == 0) {
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                reveal(r + dr, c + dc);
            }
        }
    }
}

static void reset_game(void) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            grid[r][c] = 0;
            state[r][c] = 0;
        }
    }
    cur_x = COLS / 2;
    cur_y = ROWS / 2;
    initialized = false;
    game_over = false;
    game_won = false;
}

int main(int argc, char **argv) {
    GSurface *screen = gfx_screen();
    gfx_target(screen);

    reset_game();

    int start_x = (512 - COLS * CELL) / 2;
    int start_y = 60;

    bool running = true;
    while (running) {
        if (plusos->key_poll()) {
            char ch = plusos->key_get();
            if (ch == 'q' || ch == 27) break;
            if (ch == 'a' || ch == 28) if (cur_x > 0) cur_x--;
            if (ch == 'd' || ch == 29) if (cur_x < COLS - 1) cur_x++;
            if (ch == 'w' || ch == 30) if (cur_y > 0) cur_y--;
            if (ch == 's' || ch == 31) if (cur_y < ROWS - 1) cur_y++;
            if (ch == ' ' || ch == 13) {
                if (game_over || game_won) reset_game();
                else reveal(cur_y, cur_x);
            }
            if (ch == 'f') {
                if (state[cur_y][cur_x] == 0) state[cur_y][cur_x] = 2;
                else if (state[cur_y][cur_x] == 2) state[cur_y][cur_x] = 0;
            }
        }

        /* Render */
        gfx_fill(0, 0, 512, 342, 0);

        /* Window frame */
        gfx_frame(start_x - 8, start_y - 28, start_x + COLS * CELL + 8, start_y + ROWS * CELL + 8);
        gfx_text(start_x, start_y - 22, "MINESWEEPER - PLUSOS", 1);

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                int cx = start_x + c * CELL;
                int cy = start_y + r * CELL;

                if (state[r][c] == 1) {
                    /* Revealed */
                    if (grid[r][c] == -1) {
                        gfx_fill(cx + 4, cy + 4, 8, 8, 1);
                    } else if (grid[r][c] > 0) {
                        char digit[2] = { (char)('0' + grid[r][c]), '\0' };
                        gfx_text(cx + 5, cy + 3, digit, 1);
                    }
                } else {
                    /* Hidden cell with frame */
                    gfx_frame(cx, cy, cx + CELL - 1, cy + CELL - 1);
                    if (state[r][c] == 2) {
                        /* Flag mark */
                        gfx_line(cx + 5, cy + 3, cx + 5, cy + 12, 1);
                        gfx_line(cx + 5, cy + 3, cx + 11, cy + 6, 1);
                        gfx_line(cx + 11, cy + 6, cx + 5, cy + 9, 1);
                    }
                }
            }
        }

        /* Cursor highlight */
        int hx = start_x + cur_x * CELL;
        int hy = start_y + cur_y * CELL;
        gfx_frame(hx - 1, hy - 1, hx + CELL, hy + CELL);

        if (game_over) {
            gfx_text(start_x + 60, start_y + ROWS * CELL + 15, "BOOM! GAME OVER", 1);
        } else if (game_won) {
            gfx_text(start_x + 60, start_y + ROWS * CELL + 15, "YOU WIN! ALL MINES CLEARED", 1);
        }

        sleep_ticks(2);
    }

    return 0;
}
