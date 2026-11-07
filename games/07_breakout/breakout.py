"""
07_breakout: Classic Paddle & Brick Breaker
Features angle-based ball deflection, multi-row destructible bricks,
dithered brick textures, lives, and score multiplier.
"""

import math
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, SCREEN_W, SCREEN_H

BRICK_ROWS = 6
BRICK_COLS = 14
BRICK_W = 32
BRICK_H = 12

class BreakoutGame:
    def __init__(self):
        self.win = RetroWindow(title="Breakout Arcade - PlusOS Game Engine")
        self.reset_game()

    def reset_game(self):
        self.paddle_x = SCREEN_W // 2 - 30
        self.paddle_y = SCREEN_H - 30
        self.paddle_w = 60
        self.paddle_h = 8
        self.paddle_speed = 360.0

        self.ball_x = SCREEN_W // 2
        self.ball_y = SCREEN_H - 50
        self.ball_vx = 140.0
        self.ball_vy = -200.0
        self.ball_r = 4

        self.score = 0
        self.lives = 3
        self.game_over = False

        # Build Brick Grid
        self.bricks = []
        off_x = (SCREEN_W - BRICK_COLS * (BRICK_W + 2)) // 2
        off_y = 50
        for r in range(BRICK_ROWS):
            for c in range(BRICK_COLS):
                self.bricks.append({
                    'x': off_x + c * (BRICK_W + 2),
                    'y': off_y + r * (BRICK_H + 2),
                    'w': BRICK_W,
                    'h': BRICK_H,
                    'alive': True,
                    'pts': (BRICK_ROWS - r) * 10
                })

    def update(self, dt):
        if self.game_over:
            if self.win.is_key('r') or self.win.is_key('return'):
                self.reset_game()
            self.render()
            return True

        # Paddle Movement
        if self.win.is_key('left') or self.win.is_key('a'):
            self.paddle_x -= self.paddle_speed * dt
        if self.win.is_key('right') or self.win.is_key('d'):
            self.paddle_x += self.paddle_speed * dt
        self.paddle_x = max(10, min(self.paddle_x, SCREEN_W - self.paddle_w - 10))

        # Ball Movement
        self.ball_x += self.ball_vx * dt
        self.ball_y += self.ball_vy * dt

        # Wall Collisions
        if self.ball_x <= 10 + self.ball_r:
            self.ball_x = 10 + self.ball_r
            self.ball_vx *= -1
        elif self.ball_x >= SCREEN_W - 10 - self.ball_r:
            self.ball_x = SCREEN_W - 10 - self.ball_r
            self.ball_vx *= -1

        if self.ball_y <= 30 + self.ball_r:
            self.ball_y = 30 + self.ball_r
            self.ball_vy *= -1

        # Paddle Collision
        if (self.ball_y + self.ball_r >= self.paddle_y and
            self.ball_y - self.ball_r <= self.paddle_y + self.paddle_h and
            self.ball_x >= self.paddle_x and self.ball_x <= self.paddle_x + self.paddle_w):

            # Reflect based on hit offset from paddle center
            hit_offset = (self.ball_x - (self.paddle_x + self.paddle_w / 2.0)) / (self.paddle_w / 2.0)
            speed = math.hypot(self.ball_vx, self.ball_vy)
            angle = hit_offset * (math.pi / 3.0)  # max 60 deg angle
            self.ball_vx = speed * math.sin(angle)
            self.ball_vy = -abs(speed * math.cos(angle))
            self.ball_y = self.paddle_y - self.ball_r

        # Bottom drain (lose life)
        if self.ball_y > SCREEN_H:
            self.lives -= 1
            if self.lives <= 0:
                self.game_over = True
            else:
                self.ball_x = self.paddle_x + self.paddle_w / 2.0
                self.ball_y = self.paddle_y - 20
                self.ball_vx = 140.0
                self.ball_vy = -200.0

        # Brick Collision
        for b in self.bricks:
            if b['alive']:
                if (self.ball_x + self.ball_r >= b['x'] and self.ball_x - self.ball_r <= b['x'] + b['w'] and
                    self.ball_y + self.ball_r >= b['y'] and self.ball_y - self.ball_r <= b['y'] + b['h']):
                    b['alive'] = False
                    self.ball_vy *= -1
                    self.score += b['pts']
                    break

        # Check win
        if not any(b['alive'] for b in self.bricks):
            self.reset_game()

        self.render()
        return True

    def render(self):
        self.win.clear()

        # Top Bar
        self.win.draw_text(20, 10, f"SCORE: {self.score:05d}", size=10, bold=True)
        self.win.draw_text(SCREEN_W // 2 - 30, 10, "BREAKOUT", size=10, bold=True)
        self.win.draw_text(SCREEN_W - 90, 10, f"BALLS: {'O ' * self.lives}", size=10, bold=True)
        self.win.draw_line(0, 26, SCREEN_W, 26, color="#000000")

        # Arena walls
        self.win.draw_line(10, 30, 10, SCREEN_H, color="#000000", width=2)
        self.win.draw_line(SCREEN_W - 10, 30, SCREEN_W - 10, SCREEN_H, color="#000000", width=2)

        # Draw Bricks
        for b in self.bricks:
            if b['alive']:
                self.win.draw_rect(b['x'], b['y'], b['w'], b['h'], fill="#000000", outline="")
                self.win.draw_rect(b['x'] + 1, b['y'] + 1, b['w'] - 2, b['h'] - 2, fill="#FFFFFF", outline="")
                self.win.draw_rect(b['x'] + 3, b['y'] + 3, b['w'] - 6, b['h'] - 6, fill="#000000", outline="")

        # Draw Paddle
        self.win.draw_rect(self.paddle_x, self.paddle_y, self.paddle_w, self.paddle_h, fill="#000000", outline="")
        self.win.draw_rect(self.paddle_x + 2, self.paddle_y + 2, self.paddle_w - 4, self.paddle_h - 4, fill="#FFFFFF", outline="")

        # Draw Ball
        s = self.win.scale
        self.win.canvas.create_oval(
            (self.ball_x - self.ball_r)*s, (self.ball_y - self.ball_r)*s,
            (self.ball_x + self.ball_r)*s, (self.ball_y + self.ball_r)*s,
            fill="#000000", outline=""
        )

        if self.game_over:
            self.win.draw_rect(SCREEN_W // 2 - 80, SCREEN_H // 2 - 25, 160, 50, fill="#FFFFFF", outline="#000000")
            self.win.draw_text(SCREEN_W // 2 - 60, SCREEN_H // 2 - 14, "GAME OVER", size=14, bold=True)
            self.win.draw_text(SCREEN_W // 2 - 68, SCREEN_H // 2 + 10, "Press R to Restart", size=8, bold=False)

def main():
    game = BreakoutGame()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
