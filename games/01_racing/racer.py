"""
01_racing: 3D Retro Outrun & Polygon Road Racer
Features dynamic curved roads, hills, 3D player car, AI competitors,
speedometer, tachometer, distance gauge, and collision detection.
"""

import math
import sys
import os

# Add parent directory for engine_3d
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, Vec3, Mat4, create_car, SCREEN_W, SCREEN_H

class RetroRacer:
    def __init__(self):
        self.win = RetroWindow(title="Retro 3D Racer - PlusOS Game Engine")

        # Track parameters
        self.track_length = 1600
        self.segment_length = 200
        self.rumble_length = 3
        self.road_width = 2000
        self.camera_height = 1000
        self.draw_distance = 180
        self.field_of_view = 100
        self.camera_depth = 1.0 / math.tan((self.field_of_view / 2.0) * math.pi / 180.0)

        # Build procedural track segments
        self.segments = []
        for i in range(self.track_length):
            curve = 0
            # Add curves
            if 100 < i < 300: curve = 2.5
            elif 400 < i < 600: curve = -3.0
            elif 700 < i < 900: curve = 3.5
            elif 1100 < i < 1300: curve = -2.0

            # Add hills
            y = 0
            if 200 < i < 400: y = math.sin((i - 200) / 200.0 * math.pi) * 1200
            elif 600 < i < 800: y = math.sin((i - 600) / 200.0 * math.pi) * -800
            elif 1000 < i < 1200: y = math.sin((i - 1000) / 200.0 * math.pi) * 1500

            self.segments.append({
                'index': i,
                'p1': {'world': {'x': 0, 'y': y, 'z': i * self.segment_length}, 'camera': {}, 'screen': {}},
                'p2': {'world': {'x': 0, 'y': y, 'z': (i + 1) * self.segment_length}, 'camera': {}, 'screen': {}},
                'curve': curve,
                'clip': 0
            })

        # Car & Player parameters
        self.player_x = 0.0        # -1 (left road edge) to +1 (right road edge)
        self.player_z = 0.0
        self.speed = 0.0
        self.max_speed = 12000.0
        self.accel = self.max_speed / 5.0
        self.decel = -self.max_speed / 5.0
        self.braking = -self.max_speed
        self.off_road_decel = -self.max_speed / 2.0

        # AI cars
        self.cars = [
            {'offset': -0.4, 'z': 800 * self.segment_length, 'speed': 6000.0},
            {'offset':  0.3, 'z': 1200 * self.segment_length, 'speed': 7500.0},
            {'offset': -0.2, 'z': 1500 * self.segment_length, 'speed': 5500.0},
            {'offset':  0.5, 'z': 2200 * self.segment_length, 'speed': 8000.0},
            {'offset': -0.6, 'z': 2900 * self.segment_length, 'speed': 7000.0}
        ]

        # 3D player car model
        self.car_mesh = create_car(scale=24.0)
        self.score = 0
        self.time_elapsed = 0.0

    def project(self, p, camera_x, camera_y, camera_z):
        world = p['world']
        p['camera']['x'] = world['x'] - camera_x
        p['camera']['y'] = world['y'] - camera_y
        p['camera']['z'] = world['z'] - camera_z

        if p['camera']['z'] <= 0:
            p['screen']['scale'] = 0
            return

        scale = self.camera_depth / p['camera']['z']
        p['screen']['scale'] = scale
        p['screen']['x'] = round((SCREEN_W / 2) + (scale * p['camera']['x'] * SCREEN_W / 2))
        p['screen']['y'] = round((SCREEN_H / 2) - (scale * p['camera']['y'] * SCREEN_H / 2))
        p['screen']['w'] = round(scale * self.road_width * SCREEN_W / 2)

    def update(self, dt):
        self.time_elapsed += dt

        # Player Controls
        if self.win.is_key('up') or self.win.is_key('w'):
            self.speed += self.accel * dt
        elif self.win.is_key('down') or self.win.is_key('s'):
            self.speed += self.braking * dt
        else:
            self.speed += self.decel * dt

        # Off-road penalty
        if (self.player_x < -1.0 or self.player_x > 1.0) and self.speed > self.max_speed * 0.25:
            self.speed += self.off_road_decel * dt

        # Clamp speed
        self.speed = max(0.0, min(self.speed, self.max_speed))

        # Steering
        steer = 0.0
        if self.win.is_key('left') or self.win.is_key('a'):
            steer = -1.0
        elif self.win.is_key('right') or self.win.is_key('d'):
            steer = 1.0

        speed_ratio = self.speed / self.max_speed
        self.player_x += steer * 2.5 * speed_ratio * dt

        # Advance along track
        self.player_z += self.speed * dt
        track_total_len = self.track_length * self.segment_length
        while self.player_z >= track_total_len:
            self.player_z -= track_total_len

        # Move AI cars
        for car in self.cars:
            car['z'] += car['speed'] * dt
            if car['z'] >= track_total_len:
                car['z'] -= track_total_len

            # Check collision with player
            if abs(self.player_z - car['z']) < 250 and abs(self.player_x - car['offset']) < 0.5:
                self.speed *= 0.4  # Crash slowdown

        self.score = int(self.player_z / 100.0)

        # Render Frame
        self.render()
        return True

    def render(self):
        self.win.clear()

        # Sky and horizon line
        self.win.draw_rect(0, 0, SCREEN_W, SCREEN_H // 2, fill="#FFFFFF", outline="")
        self.win.draw_line(0, SCREEN_H // 2, SCREEN_W, SCREEN_H // 2, color="#000000", width=1)

        # Distant mountain range / skyline
        for i in range(0, SCREEN_W, 40):
            h = 20 + abs((i * 13) % 35)
            self.win.draw_line(i, SCREEN_H // 2, i + 20, SCREEN_H // 2 - h, color="#555555")
            self.win.draw_line(i + 20, SCREEN_H // 2 - h, i + 40, SCREEN_H // 2, color="#555555")

        base_seg_idx = int(self.player_z / self.segment_length) % self.track_length
        base_seg = self.segments[base_seg_idx]
        player_percent = (self.player_z % self.segment_length) / float(self.segment_length)
        player_y = base_seg['p1']['world']['y'] + (base_seg['p2']['world']['y'] - base_seg['p1']['world']['y']) * player_percent

        maxy = SCREEN_H
        dx = -(base_seg['curve'] * player_percent)
        x = 0

        # Draw road segments back-to-front
        for n in range(self.draw_distance):
            seg_idx = (base_seg_idx + n) % self.track_length
            segment = self.segments[seg_idx]

            looped = segment['index'] < base_seg['index']
            cam_z = self.player_z - (self.track_length * self.segment_length if looped else 0)
            cam_x = self.player_x * self.road_width - x
            cam_y = self.camera_height + player_y

            self.project(segment['p1'], cam_x, cam_y, cam_z)
            self.project(segment['p2'], cam_x - dx, cam_y, cam_z)

            x += dx
            dx += segment['curve']

            p1 = segment['p1']['screen']
            p2 = segment['p2']['screen']

            if p1['scale'] == 0 or p2['y'] >= maxy or p2['y'] >= p1['y']:
                continue

            # Draw Grass & Road Striping
            is_odd = (seg_idx // self.rumble_length) % 2
            grass_color = "#FFFFFF" if is_odd else "#EEEEEE"
            road_color = "#CCCCCC" if is_odd else "#AAAAAA"
            rumble_color = "#000000" if is_odd else "#FFFFFF"

            s = self.win.scale
            # Grass
            self.win.canvas.create_rectangle(0, p2['y']*s, SCREEN_W*s, p1['y']*s, fill=grass_color, outline="")

            # Rumble strips
            w1 = p1['w'] * 1.15
            w2 = p2['w'] * 1.15
            self.win.canvas.create_polygon([
                (p1['x'] - w1)*s, p1['y']*s,
                (p1['x'] + w1)*s, p1['y']*s,
                (p2['x'] + w2)*s, p2['y']*s,
                (p2['x'] - w2)*s, p2['y']*s
            ], fill=rumble_color, outline="")

            # Road asphalt
            self.win.canvas.create_polygon([
                (p1['x'] - p1['w'])*s, p1['y']*s,
                (p1['x'] + p1['w'])*s, p1['y']*s,
                (p2['x'] + p2['w'])*s, p2['y']*s,
                (p2['x'] - p2['w'])*s, p2['y']*s
            ], fill=road_color, outline="#000000")

            # Center white lane dash
            if is_odd:
                lw1 = p1['w'] * 0.04
                lw2 = p2['w'] * 0.04
                self.win.canvas.create_polygon([
                    (p1['x'] - lw1)*s, p1['y']*s,
                    (p1['x'] + lw1)*s, p1['y']*s,
                    (p2['x'] + lw2)*s, p2['y']*s,
                    (p2['x'] - lw2)*s, p2['y']*s
                ], fill="#000000", outline="")

            maxy = p2['y']

        # Render 3D Player Car
        car_screen_x = SCREEN_W // 2
        car_screen_y = SCREEN_H - 55

        # Render 3D Car Model with slight tilt based on steering
        steer_tilt = 0.0
        if self.win.is_key('left') or self.win.is_key('a'): steer_tilt = 0.15
        elif self.win.is_key('right') or self.win.is_key('d'): steer_tilt = -0.15

        car_rot = Mat4.rotation_y(math.pi) @ Mat4.rotation_z(steer_tilt)
        car_trans = Mat4.translation(0, -1.2, 3.5)
        car_model = car_rot @ car_trans
        proj = Mat4.perspective(math.radians(60), SCREEN_W / SCREEN_H, 0.1, 100.0)
        view = Mat4.identity()
        self.win.render_mesh_3d(self.car_mesh, car_model, view, proj, wireframe=True, solid=True)

        # Dashboard HUD
        speed_mph = int((self.speed / self.max_speed) * 165)
        self.win.draw_rect(10, 10, 150, 48, fill="#FFFFFF", outline="#000000")
        self.win.draw_text(16, 14, f"SPEED: {speed_mph:3d} MPH", size=10, bold=True)
        self.win.draw_text(16, 28, f"SCORE: {self.score:05d}", size=10, bold=True)
        self.win.draw_text(16, 42, f"TIME : {self.time_elapsed:5.1f}s", size=10, bold=True)

        # Instructions
        self.win.draw_text(SCREEN_W - 170, 14, "ARROWS / WASD: DRIVE", size=9, bold=False)

def main():
    game = RetroRacer()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
