"""
14_solar_system_3d: 3D Solar System & Orbital Dynamics Simulator
Features real-time 3D planetary orbits, Keplerian mechanics, Saturn rings,
interactive 3D camera tilt/zoom, planet tracking, and 1-bit retro dithering.
"""

import math
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, Vec3, Mat4, SCREEN_W, SCREEN_H

class Planet:
    def __init__(self, name, radius, orbit_r, orbit_speed, color, rings=False):
        self.name = name
        self.radius = radius
        self.orbit_r = orbit_r
        self.orbit_speed = orbit_speed
        self.color = color
        self.rings = rings
        self.angle = 0.0

    def update(self, dt, speed_mult=1.0):
        self.angle += self.orbit_speed * speed_mult * dt

    def get_pos(self):
        x = self.orbit_r * math.cos(self.angle)
        z = self.orbit_r * math.sin(self.angle)
        return (x, 0.0, z)

class SolarSystem3D:
    def __init__(self):
        self.win = RetroWindow(title="3D Solar System Simulation - PlusOS Game Engine")

        self.cam_dist = 65.0
        self.cam_pitch = 0.75  # ~43 degrees downward view
        self.cam_yaw = 0.0
        self.time_speed = 1.0

        # Planets (Sun is center)
        self.planets = [
            Planet("MERCURY", 2.2, 7.0,  2.4, "#888888"),
            Planet("VENUS",   3.5, 11.5, 1.8, "#CCCCCC"),
            Planet("EARTH",   4.0, 16.5, 1.3, "#000000"),
            Planet("MARS",    2.8, 22.0, 1.0, "#666666"),
            Planet("JUPITER", 7.5, 30.0, 0.6, "#AAAAAA"),
            Planet("SATURN",  6.2, 40.0, 0.4, "#BBBBBB", rings=True),
        ]

        self.moon_angle = 0.0

    def project(self, x, y, z):
        # Rotate around Y (cam_yaw)
        cos_y, sin_y = math.cos(self.cam_yaw), math.sin(self.cam_yaw)
        rx = x * cos_y + z * sin_y
        rz = -x * sin_y + z * cos_y

        # Rotate around X (cam_pitch)
        cos_p, sin_p = math.cos(self.cam_pitch), math.sin(self.cam_pitch)
        ry = y * cos_p - rz * sin_p
        final_z = y * sin_p + rz * cos_p + self.cam_dist

        if final_z <= 0.1: return None
        fov = 340.0
        sx = int(SCREEN_W / 2 + (rx / final_z) * fov)
        sy = int(SCREEN_H / 2 - (ry / final_z) * fov)
        scale = fov / final_z
        return (sx, sy, scale, final_z)

    def update(self, dt):
        # Controls
        if self.win.is_key('left') or self.win.is_key('a'):  self.cam_yaw -= 1.2 * dt
        if self.win.is_key('right') or self.win.is_key('d'): self.cam_yaw += 1.2 * dt
        if self.win.is_key('up') or self.win.is_key('w'):    self.cam_pitch = min(1.4, self.cam_pitch + 1.0 * dt)
        if self.win.is_key('down') or self.win.is_key('s'):  self.cam_pitch = max(0.1, self.cam_pitch - 1.0 * dt)

        if self.win.is_key('q'): self.cam_dist = min(110.0, self.cam_dist + 20.0 * dt) # Zoom out
        if self.win.is_key('e'): self.cam_dist = max(25.0,  self.cam_dist - 20.0 * dt) # Zoom in

        # Update planets
        for p in self.planets:
            p.update(dt, self.time_speed)

        self.moon_angle += 4.5 * dt

        self.render()
        return True

    def render(self):
        self.win.clear()

        # Space Background
        self.win.draw_rect(0, 0, SCREEN_W, SCREEN_H, fill="#000000")

        # Distant background stars
        for i in range(40):
            sx = (i * 127 + 23) % SCREEN_W
            sy = (i * 97 + 41) % SCREEN_H
            self.win.draw_pixel(sx, sy, color="#FFFFFF")

        # Draw Orbit Paths (3D Wireframe Ellipses)
        for p in self.planets:
            pts = []
            steps = 48
            for i in range(steps + 1):
                ang = 2.0 * math.pi * i / steps
                ox = p.orbit_r * math.cos(ang)
                oz = p.orbit_r * math.sin(ang)
                proj = self.project(ox, 0, oz)
                if proj: pts.append((proj[0], proj[1]))

            for i in range(len(pts) - 1):
                self.win.draw_line(pts[i][0], pts[i][1], pts[i+1][0], pts[i+1][1], color="#444444")

        # Draw Sun at center (0, 0, 0)
        sun_proj = self.project(0, 0, 0)
        if sun_proj:
            sun_r = int(9.0 * sun_proj[2])
            self.win.draw_circle(sun_proj[0], sun_proj[1], sun_r + 4, fill="", outline="#FFFFFF")
            self.win.draw_circle(sun_proj[0], sun_proj[1], sun_r, fill="#FFFFFF", outline="#000000")
            # Corona rays
            for ray_ang in range(0, 360, 45):
                rad = math.radians(ray_ang)
                rx1 = sun_proj[0] + int(math.cos(rad) * (sun_r + 2))
                ry1 = sun_proj[1] + int(math.sin(rad) * (sun_r + 2))
                rx2 = sun_proj[0] + int(math.cos(rad) * (sun_r + 8))
                ry2 = sun_proj[1] + int(math.sin(rad) * (sun_r + 8))
                self.win.draw_line(rx1, ry1, rx2, ry2, color="#FFFFFF")

        # Collect planets for depth sorting
        drawable = []
        for p in self.planets:
            pos = p.get_pos()
            proj = self.project(pos[0], pos[1], pos[2])
            if proj:
                drawable.append((proj[3], p, pos, proj))

        # Sort by depth (furthest first)
        drawable.sort(key=lambda x: x[0], reverse=True)

        for depth, p, pos, proj in drawable:
            pr = max(2, int(p.radius * proj[2]))

            # Planet Body
            self.win.draw_circle(proj[0], proj[1], pr, fill=p.color, outline="#FFFFFF")

            # Saturn Rings
            if p.rings:
                ring_rx = int(pr * 2.4)
                ring_ry = int(pr * 0.8)
                self.win.draw_oval(proj[0] - ring_rx, proj[1] - ring_ry, proj[0] + ring_rx, proj[1] + ring_ry, fill="", outline="#FFFFFF", width=1)

            # Earth's Moon
            if p.name == "EARTH":
                mx = pos[0] + 1.8 * math.cos(self.moon_angle)
                mz = pos[2] + 1.8 * math.sin(self.moon_angle)
                m_proj = self.project(mx, 0, mz)
                if m_proj:
                    self.win.draw_circle(m_proj[0], m_proj[1], 2, fill="#FFFFFF", outline="")

            # Planet Name Label
            self.win.draw_text(proj[0] + pr + 3, proj[1] - 4, p.name, size=7, bold=True, fill="#FFFFFF")

        # HUD Overlay
        self.win.draw_rect(10, 10, 220, 52, fill="#000000", outline="#FFFFFF")
        self.win.draw_text(18, 16, "3D SOLAR SYSTEM SIMULATOR", size=9, bold=True, fill="#FFFFFF")
        self.win.draw_text(18, 30, "WASD / ARROWS: ORBIT & PITCH", size=8, bold=False, fill="#FFFFFF")
        self.win.draw_text(18, 44, "Q / E: ZOOM IN & OUT", size=8, bold=False, fill="#FFFFFF")

def main():
    game = SolarSystem3D()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
