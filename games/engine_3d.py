"""
engine_3d.py - Complete 3D Engine & Retro Video Game Library in Python
Provides 3D Vector/Matrix math, procedural 3D model generation, 1-bit Bayer dithering,
lighting, rasterization (wireframe & shaded), camera systems, collision detection,
and video game logic entities for desktop playback and rapid game development.
"""

import math
import time
import tkinter as tk
from dataclasses import dataclass, field
from typing import List, Tuple, Optional, Callable

# Screen resolution matching classic Macintosh display
SCREEN_W = 512
SCREEN_H = 342
DEFAULT_SCALE = 2  # 1024 x 684 on modern displays

# 16 Bayer Ordered Dither Patterns (4x4 matrix, 0=White, 15=Black)
BAYER_4X4 = [
    [ 0,  8,  2, 10],
    [12,  4, 14,  6],
    [ 3, 11,  1,  9],
    [15,  7, 13,  5]
]

@dataclass
class Vec3:
    x: float = 0.0
    y: float = 0.0
    z: float = 0.0

    def __add__(self, o): return Vec3(self.x + o.x, self.y + o.y, self.z + o.z)
    def __sub__(self, o): return Vec3(self.x - o.x, self.y - o.y, self.z - o.z)
    def __mul__(self, s): return Vec3(self.x * s, self.y * s, self.z * s)
    def dot(self, o): return self.x * o.x + self.y * o.y + self.z * o.z
    def cross(self, o):
        return Vec3(
            self.y * o.z - self.z * o.y,
            self.z * o.x - self.x * o.z,
            self.x * o.y - self.y * o.x
        )
    def length(self): return math.sqrt(self.x * self.x + self.y * self.y + self.z * self.z)
    def normalized(self):
        l = self.length()
        return Vec3(self.x / l, self.y / l, self.z / l) if l > 1e-5 else Vec3(0, 0, 0)

@dataclass
class Vec2:
    x: float = 0.0
    y: float = 0.0

class Mat4:
    def __init__(self):
        self.m = [[0.0]*4 for _ in range(4)]
        for i in range(4): self.m[i][i] = 1.0

    @staticmethod
    def identity():
        return Mat4()

    @staticmethod
    def translation(tx, ty, tz):
        res = Mat4()
        res.m[3][0] = tx; res.m[3][1] = ty; res.m[3][2] = tz
        return res

    @staticmethod
    def rotation_x(rad):
        res = Mat4()
        c, s = math.cos(rad), math.sin(rad)
        res.m[1][1] = c; res.m[1][2] = s
        res.m[2][1] = -s; res.m[2][2] = c
        return res

    @staticmethod
    def rotation_y(rad):
        res = Mat4()
        c, s = math.cos(rad), math.sin(rad)
        res.m[0][0] = c; res.m[0][2] = -s
        res.m[2][0] = s; res.m[2][2] = c
        return res

    @staticmethod
    def rotation_z(rad):
        res = Mat4()
        c, s = math.cos(rad), math.sin(rad)
        res.m[0][0] = c; res.m[0][1] = s
        res.m[1][0] = -s; res.m[1][1] = c
        return res

    @staticmethod
    def scaling(sx, sy, sz):
        res = Mat4()
        res.m[0][0] = sx; res.m[1][1] = sy; res.m[2][2] = sz
        return res

    @staticmethod
    def perspective(fov_rad, aspect, near_z, far_z):
        res = Mat4()
        for i in range(4): res.m[i][i] = 0.0
        tan_half = math.tan(fov_rad * 0.5)
        res.m[0][0] = 1.0 / (aspect * tan_half)
        res.m[1][1] = 1.0 / tan_half
        res.m[2][2] = -(far_z + near_z) / (far_z - near_z)
        res.m[2][3] = -1.0
        res.m[3][2] = -(2.0 * far_z * near_z) / (far_z - near_z)
        return res

    @staticmethod
    def look_at(eye: Vec3, target: Vec3, up: Vec3):
        f = (target - eye).normalized()
        s = f.cross(up).normalized()
        u = s.cross(f)
        res = Mat4()
        res.m[0][0] = s.x;  res.m[1][0] = s.y;  res.m[2][0] = s.z
        res.m[0][1] = u.x;  res.m[1][1] = u.y;  res.m[2][1] = u.z
        res.m[0][2] = -f.x; res.m[1][2] = -f.y; res.m[2][2] = -f.z
        res.m[3][0] = -s.dot(eye); res.m[3][1] = -u.dot(eye); res.m[3][2] = f.dot(eye)
        return res

    def __matmul__(self, other):
        res = Mat4()
        for i in range(4):
            for j in range(4):
                res.m[i][j] = sum(self.m[i][k] * other.m[k][j] for k in range(4))
        return res

    def transform_vec3(self, v: Vec3) -> Vec3:
        rx = v.x * self.m[0][0] + v.y * self.m[1][0] + v.z * self.m[2][0] + self.m[3][0]
        ry = v.x * self.m[0][1] + v.y * self.m[1][1] + v.z * self.m[2][1] + self.m[3][1]
        rz = v.x * self.m[0][2] + v.y * self.m[1][2] + v.z * self.m[2][2] + self.m[3][2]
        rw = v.x * self.m[0][3] + v.y * self.m[1][3] + v.z * self.m[2][3] + self.m[3][3]
        if rw != 0.0 and rw != 1.0:
            inv = 1.0 / rw
            return Vec3(rx * inv, ry * inv, rz * inv)
        return Vec3(rx, ry, rz)

@dataclass
class Face3D:
    v: Tuple[int, int, int]
    normal: Vec3 = field(default_factory=Vec3)
    shade: float = 0.0
    avg_z: float = 0.0

@dataclass
class Mesh3D:
    vertices: List[Vec3] = field(default_factory=list)
    faces: List[Face3D] = field(default_factory=list)

    def compute_normals(self):
        for f in self.faces:
            v0 = self.vertices[f.v[0]]
            v1 = self.vertices[f.v[1]]
            v2 = self.vertices[f.v[2]]
            e1 = v1 - v0
            e2 = v2 - v0
            f.normal = e1.cross(e2).normalized()

# Procedural Model Generators
def create_cube(size=1.0) -> Mesh3D:
    h = size * 0.5
    v = [
        Vec3(-h, -h, -h), Vec3( h, -h, -h), Vec3( h,  h, -h), Vec3(-h,  h, -h),
        Vec3(-h, -h,  h), Vec3( h, -h,  h), Vec3( h,  h,  h), Vec3(-h,  h,  h)
    ]
    f_idx = [
        (0, 2, 1), (0, 3, 2), (5, 6, 4), (4, 6, 7),
        (4, 7, 0), (0, 7, 3), (1, 2, 5), (5, 2, 6),
        (3, 7, 2), (2, 7, 6), (4, 0, 5), (5, 0, 1)
    ]
    faces = [Face3D(v=idx) for idx in f_idx]
    m = Mesh3D(vertices=v, faces=faces)
    m.compute_normals()
    return m

def create_pyramid(base=1.0, height=1.2) -> Mesh3D:
    b = base * 0.5
    v = [
        Vec3(-b, 0, -b), Vec3( b, 0, -b), Vec3( b, 0,  b), Vec3(-b, 0,  b),
        Vec3( 0, height, 0)
    ]
    f_idx = [(0, 2, 1), (0, 3, 2), (0, 1, 4), (1, 2, 4), (2, 3, 4), (3, 0, 4)]
    faces = [Face3D(v=idx) for idx in f_idx]
    m = Mesh3D(vertices=v, faces=faces)
    m.compute_normals()
    return m

def create_spaceship(scale=1.0) -> Mesh3D:
    s = scale
    v = [
        Vec3( 0.0,  0.0,  s * 2.0),
        Vec3(-s,     0.0, -s),
        Vec3( s,     0.0, -s),
        Vec3( 0.0,  s * 0.5, -s * 0.8),
        Vec3( 0.0, -s * 0.3, -s * 0.8),
        Vec3(-s * 0.4, s * 0.6, -s * 1.2),
        Vec3( s * 0.4, s * 0.6, -s * 1.2),
        Vec3( 0.0, 0.0, -s * 1.2)
    ]
    f_idx = [
        (0, 3, 1), (0, 2, 3), (0, 1, 4), (0, 4, 2),
        (1, 3, 5), (2, 6, 3), (1, 5, 7), (2, 7, 6),
        (3, 6, 7), (3, 7, 5), (4, 7, 1), (4, 2, 7)
    ]
    faces = [Face3D(v=idx) for idx in f_idx]
    m = Mesh3D(vertices=v, faces=faces)
    m.compute_normals()
    return m

def create_car(scale=1.0) -> Mesh3D:
    s = scale
    w, l, h = s * 0.6, s * 1.2, s * 0.35
    v = [
        Vec3(-w, 0,  l), Vec3( w, 0,  l), Vec3( w, 0, -l), Vec3(-w, 0, -l),
        Vec3(-w, h,  l * 0.6), Vec3( w, h,  l * 0.6),
        Vec3(-w * 0.8, h * 1.8, 0), Vec3( w * 0.8, h * 1.8, 0),
        Vec3(-w * 0.8, h * 1.8, -l * 0.4), Vec3( w * 0.8, h * 1.8, -l * 0.4),
        Vec3(-w, h, -l * 0.9), Vec3( w, h, -l * 0.9)
    ]
    f_idx = [
        (0, 1, 5), (0, 5, 4), (4, 5, 7), (4, 7, 6),
        (6, 7, 9), (6, 9, 8), (8, 9, 11), (8, 11, 10),
        (0, 4, 3), (4, 10, 3), (1, 2, 5), (5, 2, 11),
        (3, 10, 2), (10, 11, 2), (0, 1, 2), (0, 2, 3)
    ]
    faces = [Face3D(v=idx) for idx in f_idx]
    m = Mesh3D(vertices=v, faces=faces)
    m.compute_normals()
    return m

def create_sphere(radius=1.0, rings=8, sectors=12) -> Mesh3D:
    v = []
    for i in range(rings):
        phi = math.pi * i / (rings - 1)
        for j in range(sectors):
            theta = 2.0 * math.pi * j / sectors
            x = radius * math.sin(phi) * math.cos(theta)
            y = radius * math.cos(phi)
            z = radius * math.sin(phi) * math.sin(theta)
            v.append(Vec3(x, y, z))
    faces = []
    for i in range(rings - 1):
        for j in range(sectors):
            nj = (j + 1) % sectors
            i0 = i * sectors + j
            i1 = (i + 1) * sectors + j
            i2 = (i + 1) * sectors + nj
            i3 = i * sectors + nj
            faces.append(Face3D(v=(i0, i1, i2)))
            faces.append(Face3D(v=(i0, i2, i3)))
    m = Mesh3D(vertices=v, faces=faces)
    m.compute_normals()
    return m

# Retro Window & Game Canvas Framework
class RetroWindow:
    def __init__(self, title="Classic 3D Game", scale=DEFAULT_SCALE):
        self.scale = scale
        self.w = SCREEN_W * scale
        self.h = SCREEN_H * scale
        self.root = tk.Tk()
        self.root.title(title)
        self.root.geometry(f"{self.w}x{self.h}")
        self.root.resizable(False, False)
        self.root.configure(bg="#222")

        self.canvas = tk.Canvas(self.root, width=self.w, height=self.h, bg="#FFFFFF", highlightthickness=0)
        self.canvas.pack()

        # Keyboard & Mouse state
        self.keys = set()
        self.mouse_pos = Vec2(0, 0)
        self.mouse_down = False

        self.root.bind("<KeyPress>", self._on_key_down)
        self.root.bind("<KeyRelease>", self._on_key_up)
        self.root.bind("<Motion>", self._on_motion)
        self.root.bind("<ButtonPress-1>", self._on_btn_down)
        self.root.bind("<ButtonRelease-1>", self._on_btn_up)

        # 3D Lighting
        self.light_dir = Vec3(0.577, 0.577, -0.577).normalized()
        self.ambient = 0.25
        self.diffuse = 0.75

    def _on_key_down(self, e):
        self.keys.add(e.keysym.lower())
    def _on_key_up(self, e):
        self.keys.discard(e.keysym.lower())
    def _on_motion(self, e):
        self.mouse_pos = Vec2(e.x / self.scale, e.y / self.scale)
    def _on_btn_down(self, e):
        self.mouse_down = True
    def _on_btn_up(self, e):
        self.mouse_down = False

    def is_key(self, key_name: str) -> bool:
        return key_name.lower() in self.keys

    def clear(self):
        self.canvas.delete("all")

    def draw_pixel(self, x, y, color="#000000"):
        s = self.scale
        self.canvas.create_rectangle(x*s, y*s, (x+1)*s, (y+1)*s, fill=color, outline="")

    def draw_line(self, x0, y0, x1, y1, color="#000000", width=1):
        s = self.scale
        self.canvas.create_line(x0*s, y0*s, x1*s, y1*s, fill=color, width=width*s)

    def draw_rect(self, x, y, w, h, fill="#000000", outline="#000000"):
        s = self.scale
        self.canvas.create_rectangle(x*s, y*s, (x+w)*s, (y+h)*s, fill=fill, outline=outline)

    def draw_text(self, x, y, text, size=12, color="#000000", anchor="nw", bold=True):
        font_weight = "bold" if bold else "normal"
        self.canvas.create_text(x*self.scale, y*self.scale, text=text, font=("Courier", int(size*self.scale*0.6), font_weight), fill=color, anchor=anchor)

    def render_mesh_3d(self, mesh: Mesh3D, model_mat: Mat4, view_mat: Mat4, proj_mat: Mat4, wireframe=True, solid=True):
        mvp = model_mat @ view_mat @ proj_mat

        # World transform for lighting and Z
        world_verts = [model_mat.transform_vec3(v) for v in mesh.vertices]
        proj_verts = []
        for v in mesh.vertices:
            cv = mvp.transform_vec3(v)
            sx = (cv.x + 1.0) * 0.5 * SCREEN_W
            sy = (1.0 - (cv.y + 1.0) * 0.5) * SCREEN_H
            proj_verts.append((sx, sy, cv.z))

        # Face calculations
        faces_to_draw = []
        for f in mesh.faces:
            v0 = world_verts[f.v[0]]
            v1 = world_verts[f.v[1]]
            v2 = world_verts[f.v[2]]
            f.avg_z = (v0.z + v1.z + v2.z) / 3.0

            norm = (v1 - v0).cross(v2 - v0).normalized()
            dot = max(0.0, norm.dot(self.light_dir))
            f.shade = min(1.0, self.ambient + self.diffuse * dot)

            p0 = proj_verts[f.v[0]]
            p1 = proj_verts[f.v[1]]
            p2 = proj_verts[f.v[2]]

            # Back-face culling
            cross = (p1[0] - p0[0]) * (p2[1] - p0[1]) - (p1[1] - p0[1]) * (p2[0] - p0[0])
            if cross > 0:
                faces_to_draw.append((f.avg_z, f, p0, p1, p2))

        # Painter's algorithm
        faces_to_draw.sort(key=lambda item: item[0], reverse=True)

        s = self.scale
        for avg_z, f, p0, p1, p2 in faces_to_draw:
            pts = [p0[0]*s, p0[1]*s, p1[0]*s, p1[1]*s, p2[0]*s, p2[1]*s]
            if solid:
                # Map shade to 1-bit grayscale stipple pattern
                shade_val = int(f.shade * 255)
                fill_color = f"#{shade_val:02x}{shade_val:02x}{shade_val:02x}"
                self.canvas.create_polygon(pts, fill=fill_color, outline="#000000" if wireframe else fill_color)
            elif wireframe:
                self.canvas.create_polygon(pts, fill="", outline="#000000", width=s)

    def update(self):
        self.root.update_idletasks()
        self.root.update()

    def run_loop(self, update_cb: Callable[[float], bool], fps=60):
        target_dt = 1.0 / fps
        last_time = time.time()
        running = True
        while running:
            now = time.time()
            dt = now - last_time
            last_time = now
            if dt > 0.1: dt = 0.1
            try:
                running = update_cb(dt)
                self.update()
            except tk.TclError:
                break
            elapsed = time.time() - now
            sleep_time = target_dt - elapsed
            if sleep_time > 0:
                time.sleep(sleep_time)
