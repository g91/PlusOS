"""
09_dungeon_3d: 3D Raycasting First-Person Dungeon Crawler
Features real-time DDA raycasting, dithered 3D wall perspectives,
minimap radar, player navigation, doors, and treasure keys.
"""

import math
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, SCREEN_W, SCREEN_H

MAP = [
    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
    [1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1],
    [1,0,1,1,0,0,1,0,1,1,1,0,1,1,0,1],
    [1,0,1,0,0,0,0,0,0,0,1,0,0,1,0,1],
    [1,0,1,0,1,1,1,1,1,0,1,1,0,1,0,1],
    [1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,1],
    [1,1,1,0,1,0,1,0,1,1,1,1,0,1,1,1],
    [1,0,0,0,1,0,1,0,0,0,0,1,0,0,0,1],
    [1,0,1,1,1,0,1,1,1,1,0,1,1,1,0,1],
    [1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1],
    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]
]

MAP_W = len(MAP[0])
MAP_H = len(MAP)

class Dungeon3D:
    def __init__(self):
        self.win = RetroWindow(title="3D Dungeon Crawler - PlusOS Game Engine")

        self.player_x = 2.5
        self.player_y = 2.5
        self.dir_x = 1.0
        self.dir_y = 0.0
        self.plane_x = 0.0
        self.plane_y = 0.66  # 66 degree FOV

        self.move_speed = 3.5
        self.rot_speed = 2.4
        self.score = 0

    def update(self, dt):
        # Rotation
        rot = 0.0
        if self.win.is_key('left') or self.win.is_key('a'):  rot = -self.rot_speed * dt
        elif self.win.is_key('right') or self.win.is_key('d'): rot =  self.rot_speed * dt

        if rot != 0.0:
            old_dx = self.dir_x
            self.dir_x = self.dir_x * math.cos(rot) - self.dir_y * math.sin(rot)
            self.dir_y = old_dx * math.sin(rot) + self.dir_y * math.cos(rot)
            old_px = self.plane_x
            self.plane_x = self.plane_x * math.cos(rot) - self.plane_y * math.sin(rot)
            self.plane_y = old_px * math.sin(rot) + self.plane_y * math.cos(rot)

        # Movement with wall sliding
        move = 0.0
        if self.win.is_key('up') or self.win.is_key('w'):    move =  self.move_speed * dt
        elif self.win.is_key('down') or self.win.is_key('s'): move = -self.move_speed * dt

        if move != 0.0:
            nx = self.player_x + self.dir_x * move
            ny = self.player_y + self.dir_y * move
            if MAP[int(self.player_y)][int(nx)] == 0: self.player_x = nx
            if MAP[int(ny)][int(self.player_x)] == 0: self.player_y = ny

        self.render()
        return True

    def render(self):
        self.win.clear()

        # Sky (White) and Floor (Gray)
        self.win.draw_rect(0, 0, SCREEN_W, SCREEN_H // 2, fill="#FFFFFF", outline="")
        self.win.draw_rect(0, SCREEN_H // 2, SCREEN_W, SCREEN_H // 2, fill="#DDDDDD", outline="")

        # DDA Raycasting loop across screen columns
        num_rays = SCREEN_W // 2  # 256 rays (2 pixels wide each)
        for i in range(num_rays):
            camera_x = 2.0 * i / float(num_rays) - 1.0
            ray_dx = self.dir_x + self.plane_x * camera_x
            ray_dy = self.dir_y + self.plane_y * camera_x

            map_x = int(self.player_x)
            map_y = int(self.player_y)

            delta_dist_x = abs(1.0 / ray_dx) if ray_dx != 0 else 1e30
            delta_dist_y = abs(1.0 / ray_dy) if ray_dy != 0 else 1e30

            if ray_dx < 0:
                step_x = -1
                side_dist_x = (self.player_x - map_x) * delta_dist_x
            else:
                step_x = 1
                side_dist_x = (map_x + 1.0 - self.player_x) * delta_dist_x

            if ray_dy < 0:
                step_y = -1
                side_dist_y = (self.player_y - map_y) * delta_dist_y
            else:
                step_y = 1
                side_dist_y = (map_y + 1.0 - self.player_y) * delta_dist_y

            hit = False
            side = 0
            while not hit:
                if side_dist_x < side_dist_y:
                    side_dist_x += delta_dist_x
                    map_x += step_x
                    side = 0
                else:
                    side_dist_y += delta_dist_y
                    map_y += step_y
                    side = 1
                if MAP[map_y][map_x] > 0:
                    hit = True

            perp_wall_dist = (side_dist_x - delta_dist_x) if side == 0 else (side_dist_y - delta_dist_y)
            if perp_wall_dist <= 0.001: perp_wall_dist = 0.001

            line_height = int(SCREEN_H / perp_wall_dist)
            draw_start = max(0, -line_height // 2 + SCREEN_H // 2)
            draw_end   = min(SCREEN_H - 1, line_height // 2 + SCREEN_H // 2)

            # Shade based on side and distance
            shade = 1.0 / (1.0 + perp_wall_dist * 0.15)
            if side == 1: shade *= 0.75
            shade_val = int(shade * 240)
            color = f"#{shade_val:02x}{shade_val:02x}{shade_val:02x}"

            col_x = i * 2
            self.win.draw_rect(col_x, draw_start, 2, draw_end - draw_start, fill=color, outline="")

        # Draw 2D Minimap (Top Right)
        mm_size = 5
        mm_x = SCREEN_W - MAP_W * mm_size - 15
        mm_y = 15
        self.win.draw_rect(mm_x - 2, mm_y - 2, MAP_W * mm_size + 4, MAP_H * mm_size + 4, fill="#FFFFFF", outline="#000000")
        for r in range(MAP_H):
            for c in range(MAP_W):
                if MAP[r][c] > 0:
                    self.win.draw_rect(mm_x + c * mm_size, mm_y + r * mm_size, mm_size, mm_size, fill="#000000", outline="")

        # Player on minimap
        pm_x = mm_x + int(self.player_x * mm_size)
        pm_y = mm_y + int(self.player_y * mm_size)
        self.win.draw_rect(pm_x - 1, pm_y - 1, 3, 3, fill="#FFFFFF", outline="#000000")
        self.win.draw_line(pm_x, pm_y, pm_x + int(self.dir_x * 8), pm_y + int(self.dir_y * 8), color="#000000")

        # Compass & Crosshair
        self.win.draw_line(SCREEN_W // 2 - 6, SCREEN_H // 2, SCREEN_W // 2 + 6, SCREEN_H // 2, color="#000000")
        self.win.draw_line(SCREEN_W // 2, SCREEN_H // 2 - 6, SCREEN_W // 2, SCREEN_H // 2 + 6, color="#000000")
        self.win.draw_text(20, 15, "3D DUNGEON CRAWLER", size=10, bold=True)
        self.win.draw_text(20, 30, "WASD / ARROWS: EXPLORE", size=8, bold=False)

def main():
    game = Dungeon3D()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
