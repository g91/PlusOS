"""
02_tetris: Classic Polyomino Block Falling Puzzle
Features all 7 tetrominoes (I, J, L, O, S, T, Z), ghost piece projection,
line clearing animations, score, levels, next piece preview, and authentic retro visuals.
"""

import sys
import os
import random

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, SCREEN_W, SCREEN_H

WELL_W = 10
WELL_H = 20
BLOCK_SIZE = 14

SHAPES = {
    'I': [[(0, 1), (1, 1), (2, 1), (3, 1)], [(2, 0), (2, 1), (2, 2), (2, 3)]],
    'O': [[(1, 0), (2, 0), (1, 1), (2, 1)]],
    'T': [[(1, 0), (0, 1), (1, 1), (2, 1)], [(1, 0), (1, 1), (2, 1), (1, 2)],
          [(0, 1), (1, 1), (2, 1), (1, 2)], [(1, 0), (0, 1), (1, 1), (1, 2)]],
    'S': [[(1, 0), (2, 0), (0, 1), (1, 1)], [(1, 0), (1, 1), (2, 1), (2, 2)]],
    'Z': [[(0, 0), (1, 0), (1, 1), (2, 1)], [(2, 0), (1, 1), (2, 1), (1, 2)]],
    'J': [[(0, 0), (0, 1), (1, 1), (2, 1)], [(1, 0), (2, 0), (1, 1), (1, 2)],
          [(0, 1), (1, 1), (2, 1), (2, 2)], [(1, 0), (1, 1), (0, 2), (1, 2)]],
    'L': [[(2, 0), (0, 1), (1, 1), (2, 1)], [(1, 0), (1, 1), (1, 2), (2, 2)],
          [(0, 1), (1, 1), (2, 1), (0, 2)], [(0, 0), (1, 0), (1, 1), (1, 2)]]
}

class TetrisGame:
    def __init__(self):
        self.win = RetroWindow(title="Tetris Classic - PlusOS Game Engine")
        self.grid = [[0 for _ in range(WELL_W)] for _ in range(WELL_H)]
        self.score = 0
        self.lines = 0
        self.level = 1
        self.fall_time = 0.0
        self.fall_speed = 0.5
        self.game_over = False

        self.bag = []
        self.cur_piece = self.spawn_piece()
        self.next_piece = self.spawn_piece()

        self.last_key_time = 0.0
        self.key_delay = 0.12

    def spawn_piece(self):
        if not self.bag:
            self.bag = list(SHAPES.keys())
            random.shuffle(self.bag)
        type_ = self.bag.pop()
        return {'type': type_, 'rot': 0, 'x': 3, 'y': 0}

    def get_blocks(self, piece):
        rots = SHAPES[piece['type']]
        rot = rots[piece['rot'] % len(rots)]
        return [(piece['x'] + bx, piece['y'] + by) for bx, by in rot]

    def valid_pos(self, piece):
        for bx, by in self.get_blocks(piece):
            if bx < 0 or bx >= WELL_W or by < 0 or by >= WELL_H:
                return False
            if self.grid[by][bx] != 0:
                return False
        return True

    def lock_piece(self):
        for bx, by in self.get_blocks(self.cur_piece):
            if 0 <= by < WELL_H and 0 <= bx < WELL_W:
                self.grid[by][bx] = 1

        # Check line clears
        cleared = 0
        new_grid = [row for row in self.grid if any(cell == 0 for cell in row)]
        cleared = WELL_H - len(new_grid)
        for _ in range(cleared):
            new_grid.insert(0, [0 for _ in range(WELL_W)])
        self.grid = new_grid

        if cleared > 0:
            pts = [0, 100, 300, 500, 800]
            self.score += pts[min(cleared, 4)] * self.level
            self.lines += cleared
            self.level = 1 + self.lines // 10
            self.fall_speed = max(0.08, 0.5 - (self.level - 1) * 0.04)

        self.cur_piece = self.next_piece
        self.next_piece = self.spawn_piece()
        if not self.valid_pos(self.cur_piece):
            self.game_over = True

    def get_ghost_y(self):
        ghost = dict(self.cur_piece)
        while self.valid_pos(ghost):
            ghost['y'] += 1
        ghost['y'] -= 1
        return ghost

    def update(self, dt):
        if self.game_over:
            if self.win.is_key('r') or self.win.is_key('return'):
                self.__init__()
            self.render()
            return True

        self.fall_time += dt

        # Controls with repeat
        cur_time = self.fall_time
        if cur_time - self.last_key_time > self.key_delay:
            moved = False
            if self.win.is_key('left') or self.win.is_key('a'):
                test = dict(self.cur_piece, x=self.cur_piece['x'] - 1)
                if self.valid_pos(test): self.cur_piece = test; moved = True
            elif self.win.is_key('right') or self.win.is_key('d'):
                test = dict(self.cur_piece, x=self.cur_piece['x'] + 1)
                if self.valid_pos(test): self.cur_piece = test; moved = True
            elif self.win.is_key('up') or self.win.is_key('w') or self.win.is_key('x'):
                test = dict(self.cur_piece, rot=self.cur_piece['rot'] + 1)
                if self.valid_pos(test): self.cur_piece = test; moved = True
            elif self.win.is_key('down') or self.win.is_key('s'):
                test = dict(self.cur_piece, y=self.cur_piece['y'] + 1)
                if self.valid_pos(test): self.cur_piece = test; moved = True
            elif self.win.is_key('space'):
                self.cur_piece = self.get_ghost_y()
                self.lock_piece()
                self.fall_time = 0
                moved = True

            if moved:
                self.last_key_time = cur_time

        # Gravity
        if self.fall_time >= self.fall_speed:
            self.fall_time = 0
            test = dict(self.cur_piece, y=self.cur_piece['y'] + 1)
            if self.valid_pos(test):
                self.cur_piece = test
            else:
                self.lock_piece()

        self.render()
        return True

    def render(self):
        self.win.clear()

        # Center well
        well_x = (SCREEN_W - WELL_W * BLOCK_SIZE) // 2
        well_y = 25

        # Well frame
        self.win.draw_rect(well_x - 3, well_y - 3, WELL_W * BLOCK_SIZE + 6, WELL_H * BLOCK_SIZE + 6, fill="#EEEEEE", outline="#000000")
        self.win.draw_rect(well_x, well_y, WELL_W * BLOCK_SIZE, WELL_H * BLOCK_SIZE, fill="#FFFFFF", outline="#000000")

        # Draw locked blocks
        for y in range(WELL_H):
            for x in range(WELL_W):
                if self.grid[y][x]:
                    bx = well_x + x * BLOCK_SIZE
                    by = well_y + y * BLOCK_SIZE
                    self.win.draw_rect(bx + 1, by + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2, fill="#000000", outline="")
                    self.win.draw_rect(bx + 3, by + 3, BLOCK_SIZE - 6, BLOCK_SIZE - 6, fill="#FFFFFF", outline="")

        # Draw Ghost piece
        if not self.game_over:
            ghost = self.get_ghost_y()
            for bx, by in self.get_blocks(ghost):
                px = well_x + bx * BLOCK_SIZE
                py = well_y + by * BLOCK_SIZE
                self.win.draw_rect(px + 2, py + 2, BLOCK_SIZE - 4, BLOCK_SIZE - 4, fill="", outline="#888888")

            # Draw Falling piece
            for bx, by in self.get_blocks(self.cur_piece):
                px = well_x + bx * BLOCK_SIZE
                py = well_y + by * BLOCK_SIZE
                self.win.draw_rect(px + 1, py + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2, fill="#000000", outline="")
                self.win.draw_rect(px + 3, py + 3, BLOCK_SIZE - 6, BLOCK_SIZE - 6, fill="#AAAAAA", outline="")

        # Score & Stats Panel (Left)
        self.win.draw_rect(20, 25, 120, 160, fill="#FFFFFF", outline="#000000")
        self.win.draw_text(30, 35, "TETRIS", size=14, bold=True)
        self.win.draw_line(25, 60, 135, 60, color="#000000")
        self.win.draw_text(28, 70, f"SCORE:\n{self.score:06d}", size=10, bold=True)
        self.win.draw_text(28, 110, f"LINES:\n{self.lines:03d}", size=10, bold=True)
        self.win.draw_text(28, 145, f"LEVEL:\n{self.level:02d}", size=10, bold=True)

        # Next Piece Panel (Right)
        self.win.draw_rect(SCREEN_W - 140, 25, 120, 100, fill="#FFFFFF", outline="#000000")
        self.win.draw_text(SCREEN_W - 130, 35, "NEXT PIECE", size=10, bold=True)
        self.win.draw_line(SCREEN_W - 135, 55, SCREEN_W - 25, 55, color="#000000")
        for bx, by in SHAPES[self.next_piece['type']][0]:
            px = SCREEN_W - 110 + bx * 14
            py = 70 + by * 14
            self.win.draw_rect(px, py, 12, 12, fill="#000000", outline="")

        # Instructions (Bottom Right)
        self.win.draw_rect(SCREEN_W - 140, 140, 120, 120, fill="#FFFFFF", outline="#000000")
        self.win.draw_text(SCREEN_W - 132, 148, "CONTROLS:", size=8, bold=True)
        self.win.draw_text(SCREEN_W - 132, 165, "A/D : Left/Right\nW/X : Rotate\nS   : Soft Drop\nSPC : Hard Drop\nR   : Restart", size=8, bold=False)

        if self.game_over:
            self.win.draw_rect(well_x + 10, well_y + 100, WELL_W * BLOCK_SIZE - 20, 60, fill="#FFFFFF", outline="#000000")
            self.win.draw_text(well_x + 22, well_y + 112, "GAME OVER", size=14, bold=True)
            self.win.draw_text(well_x + 20, well_y + 138, "Press R to Restart", size=8, bold=False)

def main():
    game = TetrisGame()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
