"""
06_snake: Classic Snake & 3D Tron Lightcycle
Features smooth grid navigation, growing tail, food generation,
speed scaling, and high score tracking.
"""

import random
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, SCREEN_W, SCREEN_H

GRID_COLS = 36
GRID_ROWS = 24
CELL_SZ = 12

class SnakeGame:
    def __init__(self):
        self.win = RetroWindow(title="Snake Retro - PlusOS Game Engine")
        self.reset_game()

    def reset_game(self):
        self.snake = [(18, 12), (17, 12), (16, 12)]
        self.dir = (1, 0)
        self.next_dir = (1, 0)
        self.spawn_food()
        self.score = 0
        self.speed = 10.0
        self.move_timer = 0.0
        self.game_over = False

    def spawn_food(self):
        while True:
            self.food = (random.randint(1, GRID_COLS - 2), random.randint(1, GRID_ROWS - 2))
            if self.food not in self.snake:
                break

    def update(self, dt):
        if self.game_over:
            if self.win.is_key('r') or self.win.is_key('return'):
                self.reset_game()
            self.render()
            return True

        # Input
        if (self.win.is_key('left') or self.win.is_key('a')) and self.dir != (1, 0):
            self.next_dir = (-1, 0)
        elif (self.win.is_key('right') or self.win.is_key('d')) and self.dir != (-1, 0):
            self.next_dir = (1, 0)
        elif (self.win.is_key('up') or self.win.is_key('w')) and self.dir != (0, 1):
            self.next_dir = (0, -1)
        elif (self.win.is_key('down') or self.win.is_key('s')) and self.dir != (0, -1):
            self.next_dir = (0, 1)

        self.move_timer += dt
        if self.move_timer >= 1.0 / self.speed:
            self.move_timer = 0.0
            self.dir = self.next_dir

            head = (self.snake[0][0] + self.dir[0], self.snake[0][1] + self.dir[1])

            # Check Wall Collision
            if head[0] <= 0 or head[0] >= GRID_COLS - 1 or head[1] <= 0 or head[1] >= GRID_ROWS - 1:
                self.game_over = True
                return True

            # Check Self Collision
            if head in self.snake:
                self.game_over = True
                return True

            self.snake.insert(0, head)

            # Check Food
            if head == self.food:
                self.score += 50
                self.speed = min(22.0, self.speed + 0.5)
                self.spawn_food()
            else:
                self.snake.pop()

        self.render()
        return True

    def render(self):
        self.win.clear()

        off_x = (SCREEN_W - GRID_COLS * CELL_SZ) // 2
        off_y = 30

        # Draw Arena Border
        self.win.draw_rect(off_x, off_y, GRID_COLS * CELL_SZ, GRID_ROWS * CELL_SZ, fill="#FFFFFF", outline="#000000")
        self.win.draw_rect(off_x + 2, off_y + 2, GRID_COLS * CELL_SZ - 4, GRID_ROWS * CELL_SZ - 4, fill="", outline="#000000")

        # Draw Food (apple)
        fx = off_x + self.food[0] * CELL_SZ
        fy = off_y + self.food[1] * CELL_SZ
        self.win.draw_rect(fx + 2, fy + 2, CELL_SZ - 4, CELL_SZ - 4, fill="#000000", outline="")
        self.win.draw_rect(fx + 4, fy + 4, CELL_SZ - 8, CELL_SZ - 8, fill="#FFFFFF", outline="")

        # Draw Snake Body
        for idx, (bx, by) in enumerate(self.snake):
            px = off_x + bx * CELL_SZ
            py = off_y + by * CELL_SZ
            if idx == 0:
                # Head
                self.win.draw_rect(px + 1, py + 1, CELL_SZ - 2, CELL_SZ - 2, fill="#000000", outline="")
                # Eye
                self.win.draw_rect(px + 3, py + 3, 2, 2, fill="#FFFFFF", outline="")
            else:
                self.win.draw_rect(px + 2, py + 2, CELL_SZ - 4, CELL_SZ - 4, fill="#000000", outline="")

        # Top Bar
        self.win.draw_text(20, 8, f"SCORE: {self.score:05d}", size=10, bold=True)
        self.win.draw_text(SCREEN_W // 2 - 30, 8, "SNAKE RETRO", size=10, bold=True)
        self.win.draw_text(SCREEN_W - 130, 8, f"LENGTH: {len(self.snake):03d}", size=10, bold=True)

        if self.game_over:
            self.win.draw_rect(SCREEN_W // 2 - 80, SCREEN_H // 2 - 25, 160, 50, fill="#FFFFFF", outline="#000000")
            self.win.draw_text(SCREEN_W // 2 - 60, SCREEN_H // 2 - 14, "GAME OVER", size=14, bold=True)
            self.win.draw_text(SCREEN_W // 2 - 68, SCREEN_H // 2 + 10, "Press R to Restart", size=8, bold=False)

def main():
    game = SnakeGame()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
