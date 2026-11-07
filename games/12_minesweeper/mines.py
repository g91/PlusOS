"""
12_minesweeper: Classic Macintosh Minesweeper
Features guaranteed safe first click, recursive flood-fill discovery,
flag marking, timer, mine counter, and retro 1-bit Mac window styling.
"""

import random
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, SCREEN_W, SCREEN_H

GRID_COLS = 16
GRID_ROWS = 12
NUM_MINES = 25
CELL_SIZE = 20

class Minesweeper:
    def __init__(self):
        self.win = RetroWindow(title="Mac Minesweeper - PlusOS Game Engine")

        self.grid_x = (SCREEN_W - GRID_COLS * CELL_SIZE) // 2
        self.grid_y = 65

        self.mines = set()
        self.revealed = [[False for _ in range(GRID_COLS)] for _ in range(GRID_ROWS)]
        self.flagged = [[False for _ in range(GRID_COLS)] for _ in range(GRID_ROWS)]
        self.numbers = [[0 for _ in range(GRID_COLS)] for _ in range(GRID_ROWS)]

        self.cursor_c = GRID_COLS // 2
        self.cursor_r = GRID_ROWS // 2
        self.first_click = True
        self.game_over = False
        self.game_won = False
        self.timer = 0.0

        # Mouse click handling
        self.win.canvas.bind("<Button-1>", self.on_left_click)
        self.win.canvas.bind("<Button-3>", self.on_right_click)

    def generate_mines(self, safe_c, safe_r):
        candidates = [(r, c) for r in range(GRID_ROWS) for c in range(GRID_COLS) if not (r == safe_r and c == safe_c)]
        self.mines = set(random.sample(candidates, NUM_MINES))

        for r in range(GRID_ROWS):
            for c in range(GRID_COLS):
                if (r, c) in self.mines:
                    self.numbers[r][c] = -1
                else:
                    count = 0
                    for dr in (-1, 0, 1):
                        for dc in (-1, 0, 1):
                            if dr == 0 and dc == 0: continue
                            nr, nc = r + dr, c + dc
                            if (nr, nc) in self.mines:
                                count += 1
                    self.numbers[r][c] = count

    def reveal_cell(self, r, c):
        if not (0 <= r < GRID_ROWS and 0 <= c < GRID_COLS): return
        if self.revealed[r][c] or self.flagged[r][c]: return

        if self.first_click:
            self.generate_mines(c, r)
            self.first_click = False

        self.revealed[r][c] = True

        if (r, c) in self.mines:
            self.game_over = True
            # Reveal all mines
            for mr, mc in self.mines:
                self.revealed[mr][mc] = True
            return

        if self.numbers[r][c] == 0:
            for dr in (-1, 0, 1):
                for dc in (-1, 0, 1):
                    if dr != 0 or dc != 0:
                        self.reveal_cell(r + dr, c + dc)

        # Check win
        unrevealed_safe = 0
        for row in range(GRID_ROWS):
            for col in range(GRID_COLS):
                if not self.revealed[row][col] and (row, col) not in self.mines:
                    unrevealed_safe += 1
        if unrevealed_safe == 0:
            self.game_won = True
            self.game_over = True

    def toggle_flag(self, r, c):
        if not (0 <= r < GRID_ROWS and 0 <= c < GRID_COLS): return
        if not self.revealed[r][c]:
            self.flagged[r][c] = not self.flagged[r][c]

    def on_left_click(self, event):
        scale = self.win.scale
        cx = int((event.x / scale - self.grid_x) // CELL_SIZE)
        cy = int((event.y / scale - self.grid_y) // CELL_SIZE)
        if 0 <= cx < GRID_COLS and 0 <= cy < GRID_ROWS and not self.game_over:
            self.cursor_c, self.cursor_r = cx, cy
            self.reveal_cell(cy, cx)

    def on_right_click(self, event):
        scale = self.win.scale
        cx = int((event.x / scale - self.grid_x) // CELL_SIZE)
        cy = int((event.y / scale - self.grid_y) // CELL_SIZE)
        if 0 <= cx < GRID_COLS and 0 <= cy < GRID_ROWS and not self.game_over:
            self.toggle_flag(cy, cx)

    def update(self, dt):
        if not self.game_over and not self.first_click:
            self.timer += dt

        # Keyboard controls for non-mouse play
        if self.win.is_key('Left') or self.win.is_key('a'):
            self.cursor_c = max(0, self.cursor_c - 1)
        elif self.win.is_key('Right') or self.win.is_key('d'):
            self.cursor_c = min(GRID_COLS - 1, self.cursor_c + 1)
        elif self.win.is_key('Up') or self.win.is_key('w'):
            self.cursor_r = max(0, self.cursor_r - 1)
        elif self.win.is_key('Down') or self.win.is_key('s'):
            self.cursor_r = min(GRID_ROWS - 1, self.cursor_r + 1)

        if self.win.is_key('Return') or self.win.is_key('space'):
            if self.game_over:
                self.__init__()
            else:
                self.reveal_cell(self.cursor_r, self.cursor_c)

        if self.win.is_key('f'):
            self.toggle_flag(self.cursor_r, self.cursor_c)

        self.render()
        return True

    def render(self):
        self.win.clear()

        # Classic Mac window frame
        self.win.draw_rect(self.grid_x - 12, self.grid_y - 45, GRID_COLS * CELL_SIZE + 24, GRID_ROWS * CELL_SIZE + 57, fill="#FFFFFF", outline="#000000", width=2)
        # Title bar stripes
        for ty in range(self.grid_y - 41, self.grid_y - 27, 2):
            self.win.draw_line(self.grid_x - 8, ty, self.grid_x + GRID_COLS * CELL_SIZE + 8, ty, color="#000000")
        self.win.draw_rect(SCREEN_W // 2 - 50, self.grid_y - 43, 100, 16, fill="#FFFFFF", outline="#000000")
        self.win.draw_text(SCREEN_W // 2, self.grid_y - 35, "MINESWEEPER", size=8, bold=True, anchor="center")

        # Top status bar: Flag counter, smiley, timer
        flags_placed = sum(row.count(True) for row in self.flagged)
        mines_left = max(0, NUM_MINES - flags_placed)
        self.win.draw_text(self.grid_x, self.grid_y - 12, f"MINES: {mines_left:02d}", size=9, bold=True)
        self.win.draw_text(self.grid_x + GRID_COLS * CELL_SIZE - 65, self.grid_y - 12, f"TIME: {int(self.timer):03d}s", size=9, bold=True)

        # Smiley Face Status
        sm_x = SCREEN_W // 2
        sm_y = self.grid_y - 10
        self.win.draw_circle(sm_x, sm_y, 9, fill="#FFFFFF", outline="#000000")
        if self.game_won:
            self.win.draw_line(sm_x - 5, sm_y - 3, sm_x - 1, sm_y - 3, color="#000000") # Sunglasses
            self.win.draw_line(sm_x + 1, sm_y - 3, sm_x + 5, sm_y - 3, color="#000000")
            self.win.draw_line(sm_x - 4, sm_y + 3, sm_x + 4, sm_y + 3, color="#000000")
        elif self.game_over:
            self.win.draw_text(sm_x - 4, sm_y - 5, "x", size=8) # Dead eyes
            self.win.draw_text(sm_x + 2, sm_y - 5, "x", size=8)
            self.win.draw_line(sm_x - 4, sm_y + 4, sm_x + 4, sm_y + 4, color="#000000")
        else:
            self.win.draw_pixel(sm_x - 3, sm_y - 2, color="#000000")
            self.win.draw_pixel(sm_x + 3, sm_y - 2, color="#000000")
            self.win.draw_line(sm_x - 3, sm_y + 3, sm_x + 3, sm_y + 3, color="#000000")

        # Cells
        for r in range(GRID_ROWS):
            for c in range(GRID_COLS):
                cx = self.grid_x + c * CELL_SIZE
                cy = self.grid_y + r * CELL_SIZE

                if not self.revealed[r][c]:
                    # Unrevealed cell (beveled appearance)
                    self.win.draw_rect(cx, cy, CELL_SIZE, CELL_SIZE, fill="#EEEEEE", outline="#000000")
                    if self.flagged[r][c]:
                        # Draw flag
                        self.win.draw_line(cx + 6, cy + 4, cx + 6, cy + 16, color="#000000", width=2)
                        self.win.draw_polygon([(cx + 6, cy + 4), (cx + 14, cy + 8), (cx + 6, cy + 12)], fill="#000000")
                else:
                    # Revealed cell
                    self.win.draw_rect(cx, cy, CELL_SIZE, CELL_SIZE, fill="#FFFFFF", outline="#DDDDDD")
                    if (r, c) in self.mines:
                        # Draw mine
                        self.win.draw_circle(cx + CELL_SIZE // 2, cy + CELL_SIZE // 2, 5, fill="#000000")
                        self.win.draw_line(cx + 3, cy + CELL_SIZE // 2, cx + CELL_SIZE - 3, cy + CELL_SIZE // 2, color="#000000")
                        self.win.draw_line(cx + CELL_SIZE // 2, cy + 3, cx + CELL_SIZE // 2, cy + CELL_SIZE - 3, color="#000000")
                    else:
                        num = self.numbers[r][c]
                        if num > 0:
                            self.win.draw_text(cx + CELL_SIZE // 2, cy + CELL_SIZE // 2, str(num), size=10, bold=True, anchor="center")

        # Cursor focus box
        ccx = self.grid_x + self.cursor_c * CELL_SIZE
        ccy = self.grid_y + self.cursor_r * CELL_SIZE
        self.win.draw_rect(ccx - 1, ccy - 1, CELL_SIZE + 2, CELL_SIZE + 2, fill="", outline="#000000", width=2)

def main():
    game = Minesweeper()
    game.win.run_loop(game.update, fps=30)

if __name__ == "__main__":
    main()
