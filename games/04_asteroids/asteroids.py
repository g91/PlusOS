"""
04_asteroids: 3D Vector Space Rock Shooter
Features 2D/3D vector inertia physics, thrusters, rotational dynamics,
splitting floating space rocks, laser fire, hyperspace, and score tracking.
"""

import math
import random
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, SCREEN_W, SCREEN_H

class AsteroidsGame:
    def __init__(self):
        self.win = RetroWindow(title="Asteroids Vector - PlusOS Game Engine")
        self.reset_game()

    def reset_game(self):
        self.ship_x = SCREEN_W / 2
        self.ship_y = SCREEN_H / 2
        self.ship_vx = 0.0
        self.ship_vy = 0.0
        self.ship_angle = -math.pi / 2
        self.thrusting = False
        self.score = 0
        self.lives = 3
        self.game_over = False

        self.bullets = []  # [x, y, vx, vy, life]
        self.asteroids = []
        for _ in range(5):
            self.spawn_asteroid(random.choice([0, SCREEN_W]), random.choice([0, SCREEN_H]), size=3)

    def spawn_asteroid(self, x, y, size=3):
        angle = random.uniform(0, 2 * math.pi)
        speed = random.uniform(20.0, 50.0) / (size * 0.5)
        # Generate irregular polygon vertices
        num_pts = 10
        radii = [random.uniform(size * 10, size * 16) for _ in range(num_pts)]
        self.asteroids.append({
            'x': x, 'y': y,
            'vx': math.cos(angle) * speed,
            'vy': math.sin(angle) * speed,
            'size': size,
            'radii': radii,
            'rot': 0.0,
            'rot_speed': random.uniform(-1.0, 1.0)
        })

    def update(self, dt):
        if self.game_over:
            if self.win.is_key('r') or self.win.is_key('return'):
                self.reset_game()
            self.render()
            return True

        # Ship Rotation
        if self.win.is_key('left') or self.win.is_key('a'):
            self.ship_angle -= 4.5 * dt
        if self.win.is_key('right') or self.win.is_key('d'):
            self.ship_angle += 4.5 * dt

        # Thrusters
        self.thrusting = False
        if self.win.is_key('up') or self.win.is_key('w'):
            self.thrusting = True
            accel = 240.0 * dt
            self.ship_vx += math.cos(self.ship_angle) * accel
            self.ship_vy += math.sin(self.ship_angle) * accel

        # Hyperspace
        if self.win.is_key('h'):
            self.ship_x = random.uniform(50, SCREEN_W - 50)
            self.ship_y = random.uniform(50, SCREEN_H - 50)
            self.ship_vx *= 0.1
            self.ship_vy *= 0.1

        # Friction drag
        self.ship_vx *= 0.985
        self.ship_vy *= 0.985

        # Move ship
        self.ship_x = (self.ship_x + self.ship_vx * dt) % SCREEN_W
        self.ship_y = (self.ship_y + self.ship_vy * dt) % SCREEN_H

        # Fire Bullet
        if self.win.is_key('space') and len(self.bullets) < 5:
            if not getattr(self, 'space_held', False):
                bx = self.ship_x + math.cos(self.ship_angle) * 16
                by = self.ship_y + math.sin(self.ship_angle) * 16
                b_speed = 360.0
                self.bullets.append([bx, by, math.cos(self.ship_angle) * b_speed, math.sin(self.ship_angle) * b_speed, 1.2])
                self.space_held = True
        else:
            if not self.win.is_key('space'):
                self.space_held = False

        # Update Bullets
        for b in self.bullets[:]:
            b[0] = (b[0] + b[2] * dt) % SCREEN_W
            b[1] = (b[1] + b[3] * dt) % SCREEN_H
            b[4] -= dt
            if b[4] <= 0:
                self.bullets.remove(b)

        # Update Asteroids
        for ast in self.asteroids:
            ast['x'] = (ast['x'] + ast['vx'] * dt) % SCREEN_W
            ast['y'] = (ast['y'] + ast['vy'] * dt) % SCREEN_H
            ast['rot'] += ast['rot_speed'] * dt

            # Check collision with ship
            dist = math.hypot(self.ship_x - ast['x'], self.ship_y - ast['y'])
            if dist < ast['size'] * 14:
                self.lives -= 1
                self.ship_x, self.ship_y = SCREEN_W / 2, SCREEN_H / 2
                self.ship_vx, self.ship_vy = 0, 0
                if self.lives <= 0:
                    self.game_over = True

        # Check Bullet-Asteroid collisions
        for b in self.bullets[:]:
            hit = False
            for ast in self.asteroids[:]:
                dist = math.hypot(b[0] - ast['x'], b[1] - ast['y'])
                if dist < ast['size'] * 14:
                    self.bullets.remove(b)
                    self.asteroids.remove(ast)
                    self.score += (4 - ast['size']) * 50
                    if ast['size'] > 1:
                        self.spawn_asteroid(ast['x'], ast['y'], ast['size'] - 1)
                        self.spawn_asteroid(ast['x'], ast['y'], ast['size'] - 1)
                    hit = True
                    break
            if hit: continue

        if not self.asteroids:
            for _ in range(6):
                self.spawn_asteroid(random.choice([0, SCREEN_W]), random.choice([0, SCREEN_H]), size=3)

        self.render()
        return True

    def render(self):
        self.win.clear()

        # Top HUD
        self.win.draw_text(20, 10, f"SCORE: {self.score:05d}", size=10, bold=True)
        self.win.draw_text(SCREEN_W // 2 - 30, 10, "ASTEROIDS", size=10, bold=True)
        self.win.draw_text(SCREEN_W - 90, 10, f"SHIPS: {'^ ' * self.lives}", size=10, bold=True)
        self.win.draw_line(0, 26, SCREEN_W, 26, color="#000000")

        # Draw Asteroids
        for ast in self.asteroids:
            pts = []
            num = len(ast['radii'])
            for i, r in enumerate(ast['radii']):
                theta = ast['rot'] + 2 * math.pi * i / num
                px = (ast['x'] + math.cos(theta) * r) * self.win.scale
                py = (ast['y'] + math.sin(theta) * r) * self.win.scale
                pts.extend([px, py])
            self.win.canvas.create_polygon(pts, fill="#FFFFFF", outline="#000000", width=2)

        # Draw Ship
        nose_x = self.ship_x + math.cos(self.ship_angle) * 14
        nose_y = self.ship_y + math.sin(self.ship_angle) * 14
        left_x = self.ship_x + math.cos(self.ship_angle + 2.5) * 12
        left_y = self.ship_y + math.sin(self.ship_angle + 2.5) * 12
        right_x = self.ship_x + math.cos(self.ship_angle - 2.5) * 12
        right_y = self.ship_y + math.sin(self.ship_angle - 2.5) * 12

        s = self.win.scale
        ship_pts = [nose_x*s, nose_y*s, left_x*s, left_y*s, (self.ship_x - math.cos(self.ship_angle)*4)*s, (self.ship_y - math.sin(self.ship_angle)*4)*s, right_x*s, right_y*s]
        self.win.canvas.create_polygon(ship_pts, fill="#FFFFFF", outline="#000000", width=2)

        # Draw Thruster Flame
        if self.thrusting:
            fx = self.ship_x - math.cos(self.ship_angle) * 16
            fy = self.ship_y - math.sin(self.ship_angle) * 16
            flame_pts = [left_x*s, left_y*s, fx*s, fy*s, right_x*s, right_y*s]
            self.win.canvas.create_polygon(flame_pts, fill="", outline="#000000", width=1)

        # Draw Bullets
        for b in self.bullets:
            self.win.draw_rect(b[0] - 1, b[1] - 1, 3, 3, fill="#000000", outline="")

        if self.game_over:
            self.win.draw_rect(SCREEN_W // 2 - 80, SCREEN_H // 2 - 30, 160, 60, fill="#FFFFFF", outline="#000000")
            self.win.draw_text(SCREEN_W // 2 - 60, SCREEN_H // 2 - 16, "GAME OVER", size=14, bold=True)
            self.win.draw_text(SCREEN_W // 2 - 68, SCREEN_H // 2 + 10, "Press R to Restart", size=8, bold=False)

def main():
    game = AsteroidsGame()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
