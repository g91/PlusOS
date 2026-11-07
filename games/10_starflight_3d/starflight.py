"""
10_starflight_3d: 3D Rail Space Shooter (Starfox / Star Wars Trench style)
Features full 3D starfield, wireframe 3D enemy interceptors, 3D laser bolts,
shield meter, targeting reticle, score multiplier, and cockpit HUD.
"""

import math
import random
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, Vec3, Mat4, SCREEN_W, SCREEN_H

class Star:
    def __init__(self):
        self.reset()
        self.z = random.uniform(5.0, 50.0)

    def reset(self):
        self.x = random.uniform(-25.0, 25.0)
        self.y = random.uniform(-18.0, 18.0)
        self.z = 60.0

    def update(self, speed, dt):
        self.z -= speed * dt
        if self.z <= 0.5:
            self.reset()

class Laser:
    def __init__(self, x, y, z, tx, ty):
        self.x = x
        self.y = y
        self.z = z
        self.vx = (tx - x) * 2.0
        self.vy = (ty - y) * 2.0
        self.vz = 45.0
        self.alive = True

    def update(self, dt):
        self.x += self.vx * dt
        self.y += self.vy * dt
        self.z += self.vz * dt
        if self.z > 60.0:
            self.alive = False

class Enemy3D:
    def __init__(self):
        self.reset()

    def reset(self):
        self.x = random.uniform(-15.0, 15.0)
        self.y = random.uniform(-10.0, 10.0)
        self.z = random.uniform(45.0, 70.0)
        self.vx = random.uniform(-2.0, 2.0)
        self.vy = random.uniform(-1.5, 1.5)
        self.vz = random.uniform(-12.0, -18.0)
        self.alive = True
        self.radius = 1.6

    def update(self, dt):
        self.x += self.vx * dt
        self.y += self.vy * dt
        self.z += self.vz * dt
        if self.z <= 1.0:
            self.reset()

class Starflight3D:
    def __init__(self):
        self.win = RetroWindow(title="3D Starflight Rail Shooter - PlusOS Game Engine")

        self.reticle_x = SCREEN_W // 2
        self.reticle_y = SCREEN_H // 2
        self.speed = 15.0
        self.stars = [Star() for _ in range(90)]
        self.enemies = [Enemy3D() for _ in range(5)]
        self.lasers = []
        self.cooldown = 0.0

        self.score = 0
        self.shield = 100
        self.game_over = False

    def project(self, x, y, z):
        if z <= 0.1: return None
        fov = 280.0
        sx = int(SCREEN_W / 2 + (x / z) * fov)
        sy = int(SCREEN_H / 2 - (y / z) * fov)
        return (sx, sy)

    def fire_laser(self):
        if self.cooldown <= 0.0:
            # Map reticle to 3D world target
            fov = 280.0
            tx = (self.reticle_x - SCREEN_W / 2) / fov * 30.0
            ty = -(self.reticle_y - SCREEN_H / 2) / fov * 30.0
            self.lasers.append(Laser(-2.0, -1.5, 1.0, tx, ty))
            self.lasers.append(Laser( 2.0, -1.5, 1.0, tx, ty))
            self.cooldown = 0.18

    def update(self, dt):
        if self.game_over:
            if self.win.is_key('space') or self.win.is_key('Return'):
                self.__init__()
            self.render()
            return True

        if self.cooldown > 0:
            self.cooldown -= dt

        # Reticle controls
        speed = 220.0 * dt
        if self.win.is_key('left') or self.win.is_key('a'):  self.reticle_x -= speed
        if self.win.is_key('right') or self.win.is_key('d'): self.reticle_x += speed
        if self.win.is_key('up') or self.win.is_key('w'):    self.reticle_y -= speed
        if self.win.is_key('down') or self.win.is_key('s'):  self.reticle_y += speed

        self.reticle_x = max(40, min(SCREEN_W - 40, self.reticle_x))
        self.reticle_y = max(40, min(SCREEN_H - 40, self.reticle_y))

        if self.win.is_key('space'):
            self.fire_laser()

        # Update stars
        for s in self.stars:
            s.update(self.speed, dt)

        # Update lasers
        for l in self.lasers:
            l.update(dt)
        self.lasers = [l for l in self.lasers if l.alive]

        # Update enemies
        for e in self.enemies:
            e.update(dt)
            # Collision with lasers
            for l in self.lasers:
                dist = math.sqrt((e.x - l.x)**2 + (e.y - l.y)**2 + (e.z - l.z)**2)
                if dist < e.radius:
                    e.reset()
                    l.alive = False
                    self.score += 150
                    break

            # Near-collision with player
            if e.z < 2.0 and abs(e.x) < 2.0 and abs(e.y) < 1.5:
                self.shield -= 25
                e.reset()
                if self.shield <= 0:
                    self.game_over = True

        self.render()
        return True

    def render(self):
        self.win.clear()

        # Space Background (Dark)
        self.win.draw_rect(0, 0, SCREEN_W, SCREEN_H, fill="#000000")

        # Draw stars
        for s in self.stars:
            pt = self.project(s.x, s.y, s.z)
            if pt and 0 <= pt[0] < SCREEN_W and 0 <= pt[1] < SCREEN_H:
                size = 1 if s.z > 25.0 else 2
                self.win.draw_rect(pt[0], pt[1], size, size, fill="#FFFFFF")

        # Draw lasers
        for l in self.lasers:
            p1 = self.project(l.x, l.y, l.z)
            p2 = self.project(l.x, l.y, l.z + 2.5)
            if p1 and p2:
                self.win.draw_line(p1[0], p1[1], p2[0], p2[1], color="#FFFFFF", width=2)

        # Draw 3D Enemies (Wireframe Interceptors)
        for e in self.enemies:
            center = self.project(e.x, e.y, e.z)
            if not center: continue
            scale = 280.0 / e.z
            w = int(e.radius * scale)
            h = int(e.radius * 0.6 * scale)

            # Central cockpit hex/diamond
            self.win.draw_polygon([
                (center[0], center[1] - h),
                (center[0] + w, center[1]),
                (center[0], center[1] + h),
                (center[0] - w, center[1])
            ], fill="", outline="#FFFFFF")

            # Solar wings
            wing_span = int(w * 1.5)
            self.win.draw_line(center[0] - wing_span, center[1] - h, center[0] - wing_span, center[1] + h, color="#FFFFFF")
            self.win.draw_line(center[0] + wing_span, center[1] - h, center[0] + wing_span, center[1] + h, color="#FFFFFF")
            self.win.draw_line(center[0] - wing_span, center[1], center[0] - w, center[1], color="#FFFFFF")
            self.win.draw_line(center[0] + wing_span, center[1], center[0] + w, center[1], color="#FFFFFF")

        # Cockpit Frame & Crosshair
        cx, cy = int(self.reticle_x), int(self.reticle_y)
        self.win.draw_rect(cx - 10, cy - 10, 20, 20, fill="", outline="#FFFFFF")
        self.win.draw_line(cx - 16, cy, cx - 6, cy, color="#FFFFFF")
        self.win.draw_line(cx + 6, cy, cx + 16, cy, color="#FFFFFF")
        self.win.draw_line(cx, cy - 16, cx, cy - 6, color="#FFFFFF")
        self.win.draw_line(cx, cy + 6, cx, cy + 16, color="#FFFFFF")

        # Ship Cockpit Lower Braces
        self.win.draw_line(0, SCREEN_H, SCREEN_W // 4, SCREEN_H - 40, color="#FFFFFF", width=2)
        self.win.draw_line(SCREEN_W, SCREEN_H, SCREEN_W * 3 // 4, SCREEN_H - 40, color="#FFFFFF", width=2)
        self.win.draw_line(SCREEN_W // 4, SCREEN_H - 40, SCREEN_W * 3 // 4, SCREEN_H - 40, color="#FFFFFF", width=2)

        # HUD Text
        self.win.draw_text(20, 15, f"SCORE: {self.score:06d}", size=10, bold=True, fill="#FFFFFF")
        self.win.draw_text(SCREEN_W - 140, 15, f"SHIELD: {self.shield}%", size=10, bold=True, fill="#FFFFFF")
        self.win.draw_rect(SCREEN_W - 140, 30, 100, 8, fill="", outline="#FFFFFF")
        self.win.draw_rect(SCREEN_W - 140, 30, max(0, self.shield), 8, fill="#FFFFFF", outline="")

        if self.game_over:
            self.win.draw_rect(SCREEN_W // 2 - 120, SCREEN_H // 2 - 40, 240, 80, fill="#000000", outline="#FFFFFF")
            self.win.draw_text(SCREEN_W // 2, SCREEN_H // 2 - 15, "SHIP DESTROYED", size=14, bold=True, fill="#FFFFFF", anchor="center")
            self.win.draw_text(SCREEN_W // 2, SCREEN_H // 2 + 15, "PRESS SPACE TO RESTART", size=9, bold=False, fill="#FFFFFF", anchor="center")

def main():
    game = Starflight3D()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
