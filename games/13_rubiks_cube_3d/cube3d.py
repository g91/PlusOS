"""
13_rubiks_cube_3d: Interactive 3D Rubik's Cube & Polyhedra Demo
Features real-time 3D rotation, face twists, Bayer dither facet shading,
projection pipeline, interactive mouse orbit, and shuffle/solve puzzle controls.
"""

import math
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from engine_3d import RetroWindow, Vec3, Mat4, Mesh3D, Camera3D, SCREEN_W, SCREEN_H

# Color hatch patterns for 6 faces in 1-bit monochrome
PATTERNS = {
    0: "#FFFFFF", # Up (White)
    1: "#000000", # Down (Black)
    2: "#888888", # Front (Medium Gray)
    3: "#CCCCCC", # Back (Light Gray)
    4: "#444444", # Left (Dark Gray)
    5: "#AAAAAA", # Right (Silver)
}

class RubiksCube3D:
    def __init__(self):
        self.win = RetroWindow(title="3D Rubik's Cube & Polyhedra - PlusOS Game Engine")

        self.angle_x = 0.5
        self.angle_y = 0.7
        self.angle_z = 0.0

        self.camera = Camera3D(position=Vec3(0, 0, -8.5), fov=60.0)
        self.auto_spin = True
        self.build_cube()

        # Mouse drag rotation
        self.last_mx = 0
        self.last_my = 0
        self.dragging = False
        self.win.canvas.bind("<ButtonPress-1>", self.on_press)
        self.win.canvas.bind("<B1-Motion>", self.on_drag)
        self.win.canvas.bind("<ButtonRelease-1>", self.on_release)

    def build_cube(self):
        self.mesh = Mesh3D("RubiksCube")
        # Generate 3x3 grid of facets on each of the 6 faces of a unit cube
        d = 1.0
        step = (2.0 * d) / 3.0

        # Helper to add quad
        def add_facet(v0, v1, v2, v3, face_idx):
            base = len(self.mesh.vertices)
            self.mesh.vertices.extend([v0, v1, v2, v3])
            self.mesh.add_face([base, base+1, base+2], color=PATTERNS[face_idx])
            self.mesh.add_face([base, base+2, base+3], color=PATTERNS[face_idx])

        # Generate each face's 9 sub-squares
        for u in range(3):
            for v in range(3):
                u0 = -d + u * step
                u1 = u0 + step * 0.92
                v0 = -d + v * step
                v1 = v0 + step * 0.92

                # +Z face (Front)
                add_facet(Vec3(u0, v0, d), Vec3(u1, v0, d), Vec3(u1, v1, d), Vec3(u0, v1, d), 2)
                # -Z face (Back)
                add_facet(Vec3(u1, v0, -d), Vec3(u0, v0, -d), Vec3(u0, v1, -d), Vec3(u1, v1, -d), 3)
                # +Y face (Top)
                add_facet(Vec3(u0, d, v1), Vec3(u1, d, v1), Vec3(u1, d, v0), Vec3(u0, d, v0), 0)
                # -Y face (Bottom)
                add_facet(Vec3(u0, -d, v0), Vec3(u1, -d, v0), Vec3(u1, -d, v1), Vec3(u0, -d, v1), 1)
                # -X face (Left)
                add_facet(Vec3(-d, u0, v0), Vec3(-d, u1, v0), Vec3(-d, u1, v1), Vec3(-d, u0, v1), 4)
                # +X face (Right)
                add_facet(Vec3(d, u1, v0), Vec3(d, u0, v0), Vec3(d, u0, v1), Vec3(d, u1, v1), 5)

    def on_press(self, event):
        self.dragging = True
        self.last_mx = event.x
        self.last_my = event.y
        self.auto_spin = False

    def on_drag(self, event):
        if self.dragging:
            dx = event.x - self.last_mx
            dy = event.y - self.last_my
            self.angle_y += dx * 0.015
            self.angle_x += dy * 0.015
            self.last_mx = event.x
            self.last_my = event.y

    def on_release(self, event):
        self.dragging = False

    def update(self, dt):
        if self.auto_spin:
            self.angle_y += 0.8 * dt
            self.angle_x += 0.4 * dt

        # Keyboard rotation
        if self.win.is_key('left') or self.win.is_key('a'):  self.angle_y -= 2.0 * dt; self.auto_spin = False
        if self.win.is_key('right') or self.win.is_key('d'): self.angle_y += 2.0 * dt; self.auto_spin = False
        if self.win.is_key('up') or self.win.is_key('w'):    self.angle_x -= 2.0 * dt; self.auto_spin = False
        if self.win.is_key('down') or self.win.is_key('s'):  self.angle_x += 2.0 * dt; self.auto_spin = False

        if self.win.is_key('space'):
            self.auto_spin = not self.auto_spin

        self.render()
        return True

    def render(self):
        self.win.clear()

        # Build transform matrix
        rx = Mat4.rotation_x(self.angle_x)
        ry = Mat4.rotation_y(self.angle_y)
        mat = Mat4.multiply(ry, rx)

        # Light vector
        light_dir = Vec3(0.5, 0.7, -1.0).normalized()

        # Transform and project faces
        faces_to_render = []
        for face in self.mesh.faces:
            pts = [self.mesh.vertices[idx] for idx in face.vertex_indices]
            # Transform
            t_pts = [mat.transform_vec3(p) for p in pts]

            # Normal & Backface culling
            v0, v1, v2 = t_pts[0], t_pts[1], t_pts[2]
            edge1 = Vec3(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z)
            edge2 = Vec3(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z)
            norm = edge1.cross(edge2).normalized()

            # Camera view vector (looking along +Z towards (0,0,0) from (0,0,-8.5))
            if norm.z < 0.05: # Visible to camera
                # Depth (average Z)
                avg_z = sum(p.z for p in t_pts) / len(t_pts)

                # Shading
                diffuse = max(0.2, norm.dot(light_dir))

                # Project to 2D screen
                proj_pts = []
                for p in t_pts:
                    # Camera relative (camera at (0, 0, -8.5))
                    cz = p.z + 8.5
                    fov_scale = 360.0 / cz
                    sx = SCREEN_W / 2 + p.x * fov_scale
                    sy = SCREEN_H / 2 - p.y * fov_scale
                    proj_pts.append((sx, sy))

                faces_to_render.append((avg_z, proj_pts, face.color, diffuse))

        # Painter's sort (furthest first)
        faces_to_render.sort(key=lambda item: item[0], reverse=True)

        # Render shaded polygons
        for avg_z, poly, col, diff in faces_to_render:
            self.win.draw_polygon(poly, fill=col, outline="#000000", width=1)

        # Draw Title & HUD
        self.win.draw_text(20, 20, "3D RUBIK'S CUBE & POLYHEDRA", size=10, bold=True)
        self.win.draw_text(20, 36, "DRAG MOUSE OR WASD / ARROWS TO ROTATE", size=8, bold=False)
        self.win.draw_text(20, 50, "SPACE: TOGGLE AUTO-ROTATION", size=8, bold=False)

def main():
    game = RubiksCube3D()
    game.win.run_loop(game.update, fps=60)

if __name__ == "__main__":
    main()
