/* puzzle.c - Classic System 7.1 15-Tile Sliding Puzzle Desk Accessory */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <stdio.h>
#include <stdlib.h>

static int board[4][4];
static int empty_r = 3, empty_c = 3;
static int moves = 0;

static void try_slide(int r, int c)
{
    if (r < 0 || r >= 4 || c < 0 || c >= 4) return;
    int dr = abs(r - empty_r), dc = abs(c - empty_c);
    if (dr + dc == 1) {
        board[empty_r][empty_c] = board[r][c];
        board[r][c] = 0;
        empty_r = r;
        empty_c = c;
        moves++;
    }
}

static void scramble_board(void)
{
    for (int i = 0; i < 15; i++) board[i / 4][i % 4] = i + 1;
    board[3][3] = 0;
    empty_r = 3; empty_c = 3;
    for (int i = 0; i < 120; i++) {
        int dir = rand() & 3;
        int nr = empty_r + (dir == 0 ? -1 : dir == 1 ? 1 : 0);
        int nc = empty_c + (dir == 2 ? -1 : dir == 3 ? 1 : 0);
        try_slide(nr, nc);
    }
    moves = 0;
}

static int is_solved(void)
{
    for (int i = 0; i < 15; i++)
        if (board[i / 4][i % 4] != i + 1) return 0;
    return 1;
}

static void draw_puzzle(int ox, int oy)
{
    gfx_dither_rect(ox, oy, ox + 160, oy + 180, 4);
    gfx_frame(ox + 6, oy + 6, ox + 154, oy + 154);
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int tx = ox + 8 + c * 36;
            int ty = oy + 8 + r * 36;
            int v = board[r][c];
            if (v == 0) {
                gfx_dither_rect(tx, ty, tx + 34, ty + 34, 12);
            } else {
                gfx_fill(tx, ty, tx + 34, ty + 34, 0);
                gfx_frame(tx, ty, tx + 34, ty + 34);
                gfx_hline(tx + 2, tx + 34, ty + 33, 1);
                gfx_vline(tx + 33, ty + 2, ty + 34, 1);
                char s[8];
                snprintf(s, sizeof s, "%d", v);
                int tw = gfx_text_width(s);
                gfx_text(tx + 17 - tw / 2, ty + 13, s, 0);
            }
        }
    }
    char mbuf[32];
    if (is_solved())
        snprintf(mbuf, sizeof mbuf, "SOLVED IN %d! (R=NEW)", moves);
    else
        snprintf(mbuf, sizeof mbuf, "Moves: %d (R=Shuffle)", moves);
    gfx_fill(ox + 6, oy + 158, ox + 154, oy + 176, 0);
    gfx_frame(ox + 6, oy + 158, ox + 154, oy + 176);
    gfx_text(ox + 10, oy + 163, mbuf, 0);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    srand((unsigned)plusos->ticks());
    scramble_board();

    GWindow win;
    if (gwin_open(&win, "Puzzle", 160, 180) == 0) {
        gfx_target(&win.surf);
        draw_puzzle(0, 0);
        gwin_update(&win);
        GEvent ev;
        while (gwin_event(&win, &ev, 1) >= 0) {
            if (ev.type == GEV_CLOSE) break;
            if (ev.type == GEV_MOUSEDOWN) {
                int c = (ev.x - 8) / 36;
                int r = (ev.y - 8) / 36;
                try_slide(r, c);
                draw_puzzle(0, 0);
                gwin_update(&win);
            } else if (ev.type == GEV_KEY) {
                int k = ev.key;
                if (k == 'q' || k == 'Q' || k == 27) break;
                if (k == 'r' || k == 'R') scramble_board();
                if (k == 'w' || k == 30) try_slide(empty_r + 1, empty_c);
                if (k == 's' || k == 31) try_slide(empty_r - 1, empty_c);
                if (k == 'a' || k == 28) try_slide(empty_r, empty_c + 1);
                if (k == 'd' || k == 29) try_slide(empty_r, empty_c - 1);
                draw_puzzle(0, 0);
                gwin_update(&win);
            }
        }
        gwin_close(&win);
        return 0;
    }

    /* Full-screen mode */
    gfx_target(gfx_backbuffer());
    for (;;) {
        gfx_fill(0, 0, 512, 342, 0);
        gfx_text(140, 45, "PLUSOS 15-TILE PUZZLE (ARROWS/WASD, R=SHUFFLE, Q=QUIT)", 0);
        draw_puzzle(176, 80);
        gfx_present();
        int k = plusos->key_get();
        if (k == 'q' || k == 'Q' || k == 27) break;
        if (k == 'r' || k == 'R') scramble_board();
        if (k == 'w' || k == 30) try_slide(empty_r + 1, empty_c);
        if (k == 's' || k == 31) try_slide(empty_r - 1, empty_c);
        if (k == 'a' || k == 28) try_slide(empty_r, empty_c + 1);
        if (k == 'd' || k == 29) try_slide(empty_r, empty_c - 1);
    }
    return 0;
}
