"""
08_lunar_lander: Physics Thrust & Gravity Lunar Landing Simulation
Features rotational thrusters, main engine vector, fuel gauge,
procedural mountain surface, landing zones (2x, 3x, 5x), and touchdown physics.
"""

import math
import random
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, SCREEN_W, SCREEN_H

class LunarLander:
    def __init__(self):
        self.win = RetroWindow(title="Lunar Lander - PlusOS Game Engine")
        self.reset_game()

    def reset_game(self):
        self.lander_x = 60.0
        self.lander_y = 60.0
        self.vx = 25.0
        self.vy = 0.0
        self.angle = -math.pi / 2.0  # Pointing up
        self.fuel = 600.0
        self.gravity = 18.0
        self.thrust_power = 45.0
        self.thrusting = False
        self.score = 0
        self.game_state = "FLYING"  # FLYING, LANDED, CRASHED

        # Generate Terrain Points
        self.terrain = []
        x = 0
        y = SCREEN_H - random.randint(30, 80)
        self.pads = []

        # Pad 1
        pad1_x = random.randint(120, 180)
        # Pad 2
        pad2_x = random.randint(320, 390)

        while x < SCREEN_W:
            if abs(x - pad1_x) < 25:
                y = 280
                self.terrain.append((x, y))
                self.pads.append((pad1_x - 20, pad1_x + 20, y, 3))
                x += 40
            elif abs(x - pad2_x) < 20:
                y = 305
                self.terrain.append((x, y))
                self.pads.append((pad2_x - 15, pad2_x + 15, y, 5))
                x += 30
            else:
                y = max(180, min(SCREEN_H - 20, y + random.randint(-25, 25)))
                self.terrain.append((x, y))
                x += random.randint(25, 45)
        self.terrain.append((SCREEN_W, SCREEN_H - 40))

    def update(self, dt):
        if self.game_state != "FLYING":
            if self.win.is_key('r') or self.win.is_key('return'):
                self.reset_game()
            self.render()
            return True

        # Rotation
        if self.win.is_key('left') or self.win.is_key('a'):
            self.angle -= 2.2 * dt
        if self.win.is_key('right') or self.win.is_key('d'):
            self.angle += 2.2 * dt

        # Main Thrust
        self.thrusting = False
        if (self.win.is_key('up') or self.win.is_key('w') or self.win.is_key('space')) and self.fuel > 0:
            self.thrusting = True
            thrust_accel = self.thrust_power * dt
            self.vx += math.cos(self.angle) * thrust_accel
            self.vy += math.sin(self.angle) * thrust_accel
            self.fuel = max(0.0, self.fuel - 60.0 * dt)

        # Gravity
        self.vy += self.gravity * dt

        # Position
        self.lander_x += self.vx * dt
        self.lander_y += self.vy * dt

        # Check Ground Collision
        for i in range(len(self.terrain) - 1):
            x1, y1 = self.terrain[i]
            x2, y2 = self.terrain[i + 1]
            if x1 <= self.lander_x <= x2:
                # Interpolate terrain height
                t = (self.lander_x - x1) / (x2 - x1)
                ground_y = y1 + (y2 - y1) * t
                if self.lander_y + 10 >= ground_y:
                    # Touched ground: evaluate landing vs crash
                    upright = abs(self.angle - (-math.pi / 2.0)) < 0.25
                    soft_v = abs(self.vy) < 22.0
                    soft_h = abs(self.vx) < 14.0

                    # Check if on a pad
                    on_pad = False
                    multiplier = 1
                    for px1, px2, py, mult in self.pads:
                        if px1 <= self.lander_x <= px2 and abs(ground_y - py) < 5:
                            on_pad = True
                            multiplier = mult
                            break

                    if upright and soft_v and soft_h and on_pad:
                        self.game_state = "LANDED"
                        self.score += int(100 * multiplier + self.fuel * 2)
                    else:
                        self.game_state = "CRASHED"
                    break

        self.render()
        return True

    def render(self):
        self.win.clear()

        # Telemetry HUD
        self.win.draw_text(20, 10, f"SCORE: {self.score:05d}", size=10, bold=True)
        fuel_color = "#000000" if self.fuel > 100 else "#666666"
        self.win.draw_text(160, 10, f"FUEL: {int(self.fuel):04d}", size=10, bold=True, color=fuel_color)
        self.win.draw_text(300, 10, f"HORIZ SPD: {int(self.vx):+3d}", size=10, bold=True)
        vert_color = "#000000" if abs(self.vy) < 22.0 else "#888888"
        self.win.draw_text(410, 10, f"VERT SPD: {int(self.vy):+3d}", size=10, bold=True, color=vert_color)
        self.win.draw_line(0, 26, SCREEN_W, 26, color="#000000")

        # Draw Terrain
        s = self.win.scale
        pts = [0, SCREEN_H * s]
        for tx, ty in self.terrain:
            pts.extend([tx * s, ty * s])
        pts.extend([SCREEN_W * s, SCREEN_H * s])
        self.win.canvas.create_polygon(pts, fill="#FFFFFF", outline="#000000", width=2)

        # Draw Pads
        for px1, px2, py, mult in self.pads:
            self.win.draw_line(px1, py, px2, py, color="#000000", width=4)
            self.win.draw_text((px1 + px2) // 2 - 8, py + 6, f"{mult}X", size=8, bold=True)

        # Draw Lunar Module
        lx, ly = self.lander_x, self.lander_y
        c = math.cos(self.angle)
        sn = math.sin(self.angle)

        # Cabin Box
        box_pts = [
            (lx + c * 8 - sn * 8)*s, (ly + sn * 8 + c * 8)*s,
            (lx + c * 8 + sn * 8)*s, (ly + sn * 8 - c * 8)*s,
            (lx - c * 8 + sn * 8)*s, (ly - sn * 8 - c * 8)*s,
            (lx - c * 8 - sn * 8)*s, (ly - sn * 8 + c * 8)*s
        ]
        self.win.canvas.create_polygon(box_pts, fill="#FFFFFF", outline="#000000", width=2)

        # Legs
        leg1_x, leg1_y = lx - c * 8 - sn * 12, ly - sn * 8 + c * 12
        leg2_x, leg2_y = lx - c * 8 + sn * 12, ly - sn * 8 - c * 12
        self.win.draw_line(lx - c * 6, ly - sn * 6, leg1_x, leg1_y, color="#000000", width=2)
        self.win.draw_line(lx - c * 6, ly - sn * 6, leg2_x, leg2_y, color="#000000", width=2)

        # Thrust Flame
        if self.thrusting:
            fx = lx - c * 16
            fy = ly - sn * 16
            flame_pts = [
                (lx - c * 8 - sn * 4)*s, (ly - sn * 8 + c * 4)*s,
                fx*s, fy*s,
                (lx - c * 8 + sn * 4)*s, (ly - sn * 8 - c * 4)*s
            ]
            self.win.canvas.create_polygon(flame_pts, fill="", outline="#000000", width=1)

        # End States
        if self.game_state == "LANDED":
            self.win.draw_rect(SCREEN_W // 2 - 100, 100, 200, 60, fill="#FFFFFF", outline="#000000")
            self.win.draw_text(SCREEN_W // 2 - 80, 110, "PERFECT LANDING!", size=12, bold=True)
            self.win.draw_text(SCREEN_W // 2 - 75, 135, "Press R to play again", size=9, bold=False)
        elif self.game_state == "CRASHED":
            self.win.draw_rect(SCREEN_W // 2 - 90, 100, 180, 60, fill="#FFFFFF", outline="#000000")
            self.win.draw_text(SCREEN_W // 2 - 70, 110, "CRASHED ON MOON", size=12, bold=True)
            self.win.draw_text(SCREEN_W // 2 - 75, 135, "Press R to play again", size=9, bold=False)

def main():
    game = LunarLander()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
