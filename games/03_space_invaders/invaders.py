"""
03_space_invaders: Classic Arcade Space Invaders
Features 55 animated aliens marching in formation, alien bombs,
destructible shield bunkers, player cannon, mystery UFO flying saucer, and high score.
"""

import sys
import os
import random

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, SCREEN_W, SCREEN_H

# 1-bit Pixel Sprites for Aliens & Player (8x8 and 12x8)
SPRITES = {
    'cannon': [
        "   XXXXXX   ",
        "  XXXXXXXX  ",
        " XXXXXXXXXX ",
        "XXXXXXXXXXXX",
        "XXXXXXXXXXXX"
    ],
    'alien1': [  # Squid (Top row)
        ["  XXXX  ", " XXXXXX ", "XXXXXXXX", "XX XX XX", "XXXXXXXX", "  X  X  ", " X X  X ", "X X  X X"],
        ["  XXXX  ", " XXXXXX ", "XXXXXXXX", "XX XX XX", "XXXXXXXX", "  X  X  ", "  X  X  ", " X    X "]
    ],
    'alien2': [  # Crab (Middle rows)
        [" X    X ", "  X  X  ", " XXXXXX ", "XX XXX X", "XXXXXXXX", "X XXXXX ", "X X    X", "   X  X "],
        [" X    X ", "X X  X X", "XXXXXXXX", "XX XXX X", "XXXXXXXX", " XXXXXX ", "  X  X  ", " X    X "]
    ],
    'alien3': [  # Octopus (Bottom rows)
        ["  XXXX  ", " XXXXXX ", "XXXXXXXX", "XX XX XX", "XXXXXXXX", " X XX X ", "X      X", " X    X "],
        ["  XXXX  ", " XXXXXX ", "XXXXXXXX", "XX XX XX", "XXXXXXXX", "  XXXX  ", " X XX X ", "X X  X X"]
    ],
    'ufo': [
        "    XXXXXX    ",
        "  XXXXXXXXXX  ",
        " XXXXXXXXXXXX ",
        "XXX  XX  XX  X",
        " XXXXXXXXXXXX ",
        "   XX    XX   "
    ]
}

class SpaceInvaders:
    def __init__(self):
        self.win = RetroWindow(title="Space Invaders Classic - PlusOS Game Engine")
        self.reset_game()

    def reset_game(self):
        self.player_x = SCREEN_W // 2
        self.player_y = SCREEN_H - 35
        self.player_speed = 180.0
        self.bullet = None  # (x, y)
        self.score = 0
        self.lives = 3
        self.game_over = False

        # Build 5x11 Alien Grid
        self.aliens = []
        for row in range(5):
            for col in range(11):
                type_ = 'alien1' if row == 0 else ('alien2' if row < 3 else 'alien3')
                pts = 30 if row == 0 else (20 if row < 3 else 10)
                self.aliens.append({
                    'x': 50 + col * 32,
                    'y': 60 + row * 24,
                    'type': type_,
                    'pts': pts,
                    'alive': True
                })

        self.alien_dir = 1
        self.alien_step_x = 10
        self.alien_step_y = 12
        self.alien_move_timer = 0.0
        self.alien_move_delay = 0.6
        self.anim_frame = 0

        self.alien_bullets = []
        self.bullet_timer = 0.0

        # UFO
        self.ufo = None
        self.ufo_timer = 0.0

        # 4 Bunkers
        self.bunkers = []
        for i in range(4):
            bx = 70 + i * 105
            by = SCREEN_H - 75
            blocks = []
            for r in range(4):
                for c in range(6):
                    if not (r == 0 and (c == 0 or c == 5)) and not (r == 3 and (c == 2 or c == 3)):
                        blocks.append({'x': bx + c * 5, 'y': by + r * 5, 'hp': 3})
            self.bunkers.append(blocks)

    def update(self, dt):
        if self.game_over:
            if self.win.is_key('r') or self.win.is_key('return'):
                self.reset_game()
            self.render()
            return True

        # Player Movement
        if self.win.is_key('left') or self.win.is_key('a'):
            self.player_x -= self.player_speed * dt
        if self.win.is_key('right') or self.win.is_key('d'):
            self.player_x += self.player_speed * dt
        self.player_x = max(20, min(self.player_x, SCREEN_W - 35))

        # Fire Bullet
        if (self.win.is_key('space') or self.win.is_key('w')) and self.bullet is None:
            self.bullet = [self.player_x + 6, self.player_y - 4]

        # Update Player Bullet
        if self.bullet:
            self.bullet[1] -= 320.0 * dt
            if self.bullet[1] < 20:
                self.bullet = None
            else:
                # Collision with Aliens
                for a in self.aliens:
                    if a['alive'] and abs(self.bullet[0] - (a['x'] + 8)) < 10 and abs(self.bullet[1] - (a['y'] + 6)) < 8:
                        a['alive'] = False
                        self.score += a['pts']
                        self.bullet = None
                        # Speed up aliens
                        alive_cnt = sum(1 for al in self.aliens if al['alive'])
                        self.alien_move_delay = max(0.06, alive_cnt * 0.012)
                        break

                # Collision with UFO
                if self.bullet and self.ufo:
                    if abs(self.bullet[0] - (self.ufo['x'] + 10)) < 16 and abs(self.bullet[1] - self.ufo['y']) < 8:
                        self.score += random.choice([50, 100, 150, 300])
                        self.ufo = None
                        self.bullet = None

        # UFO Spawn & Move
        self.ufo_timer += dt
        if self.ufo_timer > 25.0 and not self.ufo:
            self.ufo = {'x': -30, 'y': 38, 'dir': 1}
            self.ufo_timer = 0.0

        if self.ufo:
            self.ufo['x'] += 100.0 * dt
            if self.ufo['x'] > SCREEN_W + 30:
                self.ufo = None

        # Alien Marching
        self.alien_move_timer += dt
        if self.alien_move_timer >= self.alien_move_delay:
            self.alien_move_timer = 0.0
            self.anim_frame = 1 - self.anim_frame

            # Check edge hit
            hit_edge = False
            for a in self.aliens:
                if a['alive']:
                    if (self.alien_dir == 1 and a['x'] >= SCREEN_W - 45) or (self.alien_dir == -1 and a['x'] <= 25):
                        hit_edge = True
                        break

            if hit_edge:
                self.alien_dir *= -1
                for a in self.aliens:
                    a['y'] += self.alien_step_y
                    if a['alive'] and a['y'] >= self.player_y - 10:
                        self.game_over = True
            else:
                for a in self.aliens:
                    a['x'] += self.alien_step_x * self.alien_dir

        # Alien Bomb Drops
        self.bullet_timer += dt
        if self.bullet_timer > 1.2:
            self.bullet_timer = 0.0
            alive_aliens = [a for a in self.aliens if a['alive']]
            if alive_aliens:
                shooter = random.choice(alive_aliens)
                self.alien_bullets.append([shooter['x'] + 8, shooter['y'] + 12])

        # Update Alien Bullets
        for ab in self.alien_bullets[:]:
            ab[1] += 160.0 * dt
            if ab[1] > SCREEN_H - 10:
                self.alien_bullets.remove(ab)
            # Hit player
            elif abs(ab[0] - (self.player_x + 6)) < 10 and abs(ab[1] - self.player_y) < 8:
                self.alien_bullets.remove(ab)
                self.lives -= 1
                if self.lives <= 0:
                    self.game_over = True

        # Check win condition
        if not any(a['alive'] for a in self.aliens):
            self.reset_game()

        self.render()
        return True

    def draw_sprite(self, x, y, rows, scale=2):
        for r_idx, row in enumerate(rows):
            for c_idx, char in enumerate(row):
                if char == 'X':
                    self.win.draw_rect(x + c_idx * scale, y + r_idx * scale, scale, scale, fill="#000000", outline="")

    def render(self):
        self.win.clear()

        # Score & Top Bar
        self.win.draw_text(20, 10, f"SCORE: {self.score:05d}", size=10, bold=True)
        self.win.draw_text(SCREEN_W // 2 - 40, 10, "SPACE INVADERS", size=10, bold=True)
        self.win.draw_text(SCREEN_W - 100, 10, f"LIVES: {'#' * self.lives}", size=10, bold=True)
        self.win.draw_line(0, 30, SCREEN_W, 30, color="#000000")

        # Ground line
        self.win.draw_line(0, SCREEN_H - 15, SCREEN_W, SCREEN_H - 15, color="#000000")

        # Draw UFO
        if self.ufo:
            self.draw_sprite(self.ufo['x'], self.ufo['y'], SPRITES['ufo'], scale=2)

        # Draw Aliens
        for a in self.aliens:
            if a['alive']:
                rows = SPRITES[a['type']][self.anim_frame]
                self.draw_sprite(a['x'], a['y'], rows, scale=2)

        # Draw Bunkers
        for b in self.bunkers:
            for blk in b:
                if blk['hp'] > 0:
                    color = "#000000" if blk['hp'] == 3 else ("#666666" if blk['hp'] == 2 else "#AAAAAA")
                    self.win.draw_rect(blk['x'], blk['y'], 5, 5, fill=color, outline="")

        # Draw Player
        self.draw_sprite(self.player_x, self.player_y, SPRITES['cannon'], scale=2)

        # Draw Player Bullet
        if self.bullet:
            self.win.draw_rect(self.bullet[0], self.bullet[1], 2, 7, fill="#000000", outline="")

        # Draw Alien Bullets
        for ab in self.alien_bullets:
            self.win.draw_line(ab[0], ab[1], ab[0], ab[1] + 5, color="#000000", width=2)

        if self.game_over:
            self.win.draw_rect(SCREEN_W // 2 - 90, SCREEN_H // 2 - 30, 180, 60, fill="#FFFFFF", outline="#000000")
            self.win.draw_text(SCREEN_W // 2 - 70, SCREEN_H // 2 - 18, "GAME OVER", size=14, bold=True)
            self.win.draw_text(SCREEN_W // 2 - 75, SCREEN_H // 2 + 8, "Press R to Restart", size=9, bold=False)

def main():
    game = SpaceInvaders()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
