"""
11_flappy_jumper: Retro Arcade Flap Jumper
Features precise jump impulse physics, procedural scrolling pipe obstacles,
score counter, medal awards, and 1-bit Mac aesthetic.
"""

import random
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, SCREEN_W, SCREEN_H

class Pipe:
    def __init__(self, x):
        self.x = x
        self.width = 46
        self.gap = 85
        self.top_h = random.randint(40, SCREEN_H - 140)
        self.passed = False

    def update(self, speed, dt):
        self.x -= speed * dt

    def collides(self, px, py, pr):
        # Top pipe
        if (px + pr > self.x) and (px - pr < self.x + self.width):
            if py - pr < self.top_h:
                return True
            if py + pr > self.top_h + self.gap:
                return True
        return False

class FlappyJumper:
    def __init__(self):
        self.win = RetroWindow(title="Retro Flap Jumper - PlusOS Game Engine")

        self.player_x = 90
        self.player_y = SCREEN_H // 2
        self.player_vy = 0.0
        self.player_r = 10

        self.gravity = 650.0
        self.jump_impulse = -230.0
        self.speed = 120.0

        self.pipes = [Pipe(SCREEN_W + i * 180) for i in range(4)]
        self.score = 0
        self.game_over = False
        self.ground_offset = 0.0

    def update(self, dt):
        if self.game_over:
            if self.win.is_key('space') or self.win.is_key('Return'):
                self.__init__()
            self.render()
            return True

        # Flap jump input
        if self.win.is_key('space') or self.win.is_key('up') or self.win.is_key('w'):
            self.player_vy = self.jump_impulse

        # Physics
        self.player_vy += self.gravity * dt
        self.player_y += self.player_vy * dt

        # Ground / Ceiling check
        ground_y = SCREEN_H - 35
        if self.player_y - self.player_r <= 0:
            self.player_y = self.player_r
            self.player_vy = 0
        if self.player_y + self.player_r >= ground_y:
            self.player_y = ground_y - self.player_r
            self.game_over = True

        # Ground scroll
        self.ground_offset = (self.ground_offset + self.speed * dt) % 16

        # Pipes
        for p in self.pipes:
            p.update(self.speed, dt)
            if not p.passed and p.x + p.width < self.player_x:
                p.passed = True
                self.score += 1

            if p.collides(self.player_x, self.player_y, self.player_r):
                self.game_over = True

        if self.pipes[0].x + self.pipes[0].width < -10:
            last_x = self.pipes[-1].x
            self.pipes.pop(0)
            self.pipes.append(Pipe(last_x + 180))

        self.render()
        return True

    def render(self):
        self.win.clear()

        # Sky background (White)
        self.win.draw_rect(0, 0, SCREEN_W, SCREEN_H, fill="#FFFFFF")

        # Distant dithered clouds
        self.win.draw_circle(120, 70, 24, fill="#EEEEEE", outline="#000000")
        self.win.draw_circle(145, 60, 30, fill="#EEEEEE", outline="#000000")
        self.win.draw_circle(170, 70, 24, fill="#EEEEEE", outline="#000000")

        self.win.draw_circle(360, 95, 20, fill="#EEEEEE", outline="#000000")
        self.win.draw_circle(385, 85, 26, fill="#EEEEEE", outline="#000000")
        self.win.draw_circle(410, 95, 20, fill="#EEEEEE", outline="#000000")

        # Pipes
        ground_y = SCREEN_H - 35
        for p in self.pipes:
            px = int(p.x)
            pw = p.width
            # Top pipe body & cap
            self.win.draw_rect(px, 0, pw, p.top_h - 14, fill="#CCCCCC", outline="#000000", width=2)
            self.win.draw_rect(px - 4, p.top_h - 14, pw + 8, 14, fill="#FFFFFF", outline="#000000", width=2)

            # Bottom pipe body & cap
            by = p.top_h + p.gap
            bh = ground_y - by
            self.win.draw_rect(px - 4, by, pw + 8, 14, fill="#FFFFFF", outline="#000000", width=2)
            self.win.draw_rect(px, by + 14, pw, bh - 14, fill="#CCCCCC", outline="#000000", width=2)

        # Ground
        self.win.draw_rect(0, ground_y, SCREEN_W, 35, fill="#EEEEEE", outline="#000000", width=2)
        for gx in range(-16, SCREEN_W + 16, 16):
            sx = gx - int(self.ground_offset)
            self.win.draw_line(sx, ground_y, sx + 8, SCREEN_H, color="#000000")

        # Draw Player Bird (Classic retro shape)
        bx, by = int(self.player_x), int(self.player_y)
        # Bird body
        self.win.draw_circle(bx, by, self.player_r, fill="#FFFFFF", outline="#000000", width=2)
        # Wing
        self.win.draw_polygon([(bx - 7, by), (bx, by + 5), (bx - 2, by)], fill="#000000")
        # Eye
        self.win.draw_circle(bx + 4, by - 3, 2, fill="#000000")
        # Beak
        self.win.draw_polygon([(bx + 8, by - 2), (bx + 14, by + 1), (bx + 8, by + 4)], fill="#000000")

        # Score Banner
        self.win.draw_text(SCREEN_W // 2, 25, f"{self.score}", size=20, bold=True, fill="#000000", anchor="center")

        if self.game_over:
            self.win.draw_rect(SCREEN_W // 2 - 110, SCREEN_H // 2 - 45, 220, 90, fill="#FFFFFF", outline="#000000", width=2)
            self.win.draw_text(SCREEN_W // 2, SCREEN_H // 2 - 25, "GAME OVER", size=14, bold=True, fill="#000000", anchor="center")
            self.win.draw_text(SCREEN_W // 2, SCREEN_H // 2, f"FINAL SCORE: {self.score}", size=10, bold=True, fill="#000000", anchor="center")
            self.win.draw_text(SCREEN_W // 2, SCREEN_H // 2 + 25, "PRESS SPACE TO RETRY", size=8, bold=False, fill="#000000", anchor="center")

def main():
    game = FlappyJumper()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
