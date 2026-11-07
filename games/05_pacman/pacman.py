"""
05_pacman: Classic Arcade Maze Chaser
Features full maze navigation, pellets, power energizers, 4 chasing ghosts
with scatter/chase AI, frightened vulnerable mode, and high score.
"""

import math
import random
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, SCREEN_W, SCREEN_H

MAZE = [
    "############################",
    "#............##............#",
    "#.####.#####.##.#####.####.#",
    "#o####.#####.##.#####.####o#",
    "#.####.#####.##.#####.####.#",
    "#..........................#",
    "#.####.##.########.##.####.#",
    "#.####.##.########.##.####.#",
    "#......##....##....##......#",
    "######.##### ## #####.######",
    "     #.##### ## #####.#     ",
    "     #.##          ##.#     ",
    "     #.## ###--### ##.#     ",
    "######.## #      # ##.######",
    "      .   #      #   .      ",
    "######.## ####### ##.######",
    "     #.##          ##.#     ",
    "     #.## ######## ##.#     ",
    "     #.## ######## ##.#     ",
    "######.##    ##    ##.######",
    "#............##............#",
    "#.####.#####.##.#####.####.#",
    "#.####.#####.##.#####.####.#",
    "#o..##................##..o#",
    "###.##.##.########.##.##.###",
    "###.##.##.########.##.##.###",
    "#......##....##....##......#",
    "#.##########.##.##########.#",
    "#.##########.##.##########.#",
    "#..........................#",
    "############################"
]

GRID_W = len(MAZE[0])
GRID_H = len(MAZE)
TILE_SZ = 10

class PacmanGame:
    def __init__(self):
        self.win = RetroWindow(title="Pac-Man Arcade - PlusOS Game Engine")
        self.reset_game()

    def reset_game(self):
        self.score = 0
        self.lives = 3
        self.game_over = False

        # Load dots
        self.dots = set()
        self.energizers = set()
        for r in range(GRID_H):
            for c in range(GRID_W):
                ch = MAZE[r][c]
                if ch == '.': self.dots.add((c, r))
                elif ch == 'o': self.energizers.add((c, r))

        self.pac_x, self.pac_y = 13.5, 23.0
        self.pac_dir = (-1, 0)
        self.next_dir = (-1, 0)
        self.pac_speed = 7.5
        self.mouth_angle = 0.0

        # Ghosts: Blinky, Pinky, Inky, Clyde
        self.ghosts = [
            {'x': 13.5, 'y': 11.0, 'dir': (0, -1), 'name': 'Blinky', 'scared': 0.0},
            {'x': 11.5, 'y': 14.0, 'dir': (0, 1),  'name': 'Pinky',  'scared': 0.0},
            {'x': 13.5, 'y': 14.0, 'dir': (0, -1), 'name': 'Inky',   'scared': 0.0},
            {'x': 15.5, 'y': 14.0, 'dir': (0, 1),  'name': 'Clyde',  'scared': 0.0}
        ]

    def is_wall(self, c, r):
        c = int(c) % GRID_W
        r = int(r)
        if 0 <= r < GRID_H and 0 <= c < GRID_W:
            return MAZE[r][c] == '#' or MAZE[r][c] == '-'
        return True

    def update(self, dt):
        if self.game_over:
            if self.win.is_key('r') or self.win.is_key('return'):
                self.reset_game()
            self.render()
            return True

        # Controls
        if self.win.is_key('left') or self.win.is_key('a'):  self.next_dir = (-1, 0)
        elif self.win.is_key('right') or self.win.is_key('d'): self.next_dir = (1, 0)
        elif self.win.is_key('up') or self.win.is_key('w'):    self.next_dir = (0, -1)
        elif self.win.is_key('down') or self.win.is_key('s'):  self.next_dir = (0, 1)

        # Pacman Movement with grid turning
        test_x = self.pac_x + self.next_dir[0] * 0.4
        test_y = self.pac_y + self.next_dir[1] * 0.4
        if not self.is_wall(round(test_x), round(test_y)):
            self.pac_dir = self.next_dir

        nx = self.pac_x + self.pac_dir[0] * self.pac_speed * dt
        ny = self.pac_y + self.pac_dir[1] * self.pac_speed * dt

        if not self.is_wall(nx + self.pac_dir[0] * 0.4, ny + self.pac_dir[1] * 0.4):
            self.pac_x, self.pac_y = nx, ny
            self.mouth_angle += 15.0 * dt
        self.pac_x = (self.pac_x + GRID_W) % GRID_W

        # Eat dots
        cell = (round(self.pac_x), round(self.pac_y))
        if cell in self.dots:
            self.dots.remove(cell)
            self.score += 10
        elif cell in self.energizers:
            self.energizers.remove(cell)
            self.score += 50
            for g in self.ghosts:
                g['scared'] = 8.0  # 8 seconds of frightened mode

        # Move Ghosts
        dirs = [(-1, 0), (1, 0), (0, -1), (0, 1)]
        for g in self.ghosts:
            if g['scared'] > 0: g['scared'] -= dt
            speed = 4.5 if g['scared'] > 0 else 6.5
            gx = g['x'] + g['dir'][0] * speed * dt
            gy = g['y'] + g['dir'][1] * speed * dt

            # At tile intersection, pick direction towards Pacman or random
            if self.is_wall(gx + g['dir'][0] * 0.4, gy + g['dir'][1] * 0.4) or random.random() < 0.05:
                valid_dirs = [d for d in dirs if not self.is_wall(round(g['x'] + d[0]), round(g['y'] + d[1])) and (d[0] != -g['dir'][0] or d[1] != -g['dir'][1])]
                if valid_dirs:
                    if g['scared'] > 0:
                        g['dir'] = random.choice(valid_dirs)
                    else:
                        # Target Pacman
                        valid_dirs.sort(key=lambda d: math.hypot((g['x'] + d[0]) - self.pac_x, (g['y'] + d[1]) - self.pac_y))
                        g['dir'] = valid_dirs[0]
            else:
                g['x'], g['y'] = gx, gy

            # Check collision with Pacman
            if math.hypot(g['x'] - self.pac_x, g['y'] - self.pac_y) < 0.8:
                if g['scared'] > 0:
                    self.score += 200
                    g['x'], g['y'] = 13.5, 14.0
                    g['scared'] = 0.0
                else:
                    self.lives -= 1
                    self.pac_x, self.pac_y = 13.5, 23.0
                    if self.lives <= 0:
                        self.game_over = True

        self.render()
        return True

    def render(self):
        self.win.clear()

        offset_x = (SCREEN_W - GRID_W * TILE_SZ) // 2
        offset_y = 16

        # Draw Maze Walls
        for r in range(GRID_H):
            for c in range(GRID_W):
                x = offset_x + c * TILE_SZ
                y = offset_y + r * TILE_SZ
                ch = MAZE[r][c]
                if ch == '#':
                    self.win.draw_rect(x, y, TILE_SZ, TILE_SZ, fill="#000000", outline="")
                elif ch == '-':
                    self.win.draw_rect(x, y + 4, TILE_SZ, 2, fill="#888888", outline="")

        # Draw Dots
        for c, r in self.dots:
            px = offset_x + c * TILE_SZ + 4
            py = offset_y + r * TILE_SZ + 4
            self.win.draw_rect(px, py, 2, 2, fill="#000000", outline="")

        # Draw Energizers
        for c, r in self.energizers:
            px = offset_x + c * TILE_SZ + 2
            py = offset_y + r * TILE_SZ + 2
            self.win.draw_rect(px, py, 6, 6, fill="#000000", outline="")

        # Draw Pacman
        px = offset_x + self.pac_x * TILE_SZ + 5
        py = offset_y + self.pac_y * TILE_SZ + 5
        pr = 5
        s = self.win.scale
        self.win.canvas.create_oval((px - pr)*s, (py - pr)*s, (px + pr)*s, (py + pr)*s, fill="#000000", outline="")

        # Draw Ghosts
        for g in self.ghosts:
            gx = offset_x + g['x'] * TILE_SZ + 5
            gy = offset_y + g['y'] * TILE_SZ + 5
            gr = 5
            fill_c = "#666666" if g['scared'] > 0 else "#000000"
            self.win.canvas.create_oval((gx - gr)*s, (gy - gr)*s, (gx + gr)*s, (gy + gr)*s, fill=fill_c, outline="")
            # Eyes
            self.win.canvas.create_oval((gx - 2)*s, (gy - 2)*s, (gx)*s, (gy)*s, fill="#FFFFFF", outline="")
            self.win.canvas.create_oval((gx + 1)*s, (gy - 2)*s, (gx + 3)*s, (gy)*s, fill="#FFFFFF", outline="")

        # Side HUD
        self.win.draw_rect(10, 20, 100, 80, fill="#FFFFFF", outline="#000000")
        self.win.draw_text(16, 26, "PAC-MAN", size=10, bold=True)
        self.win.draw_text(16, 46, f"SCORE:\n{self.score:05d}", size=9, bold=True)
        self.win.draw_text(16, 75, f"LIVES: {'C ' * self.lives}", size=9, bold=True)

        if self.game_over:
            self.win.draw_rect(SCREEN_W // 2 - 80, SCREEN_H // 2 - 25, 160, 50, fill="#FFFFFF", outline="#000000")
            self.win.draw_text(SCREEN_W // 2 - 60, SCREEN_H // 2 - 14, "GAME OVER", size=14, bold=True)
            self.win.draw_text(SCREEN_W // 2 - 68, SCREEN_H // 2 + 10, "Press R to Restart", size=8, bold=False)

def main():
    game = PacmanGame()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
