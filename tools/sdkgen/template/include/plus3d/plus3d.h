/* plus3d.h - Complete 3D Engine & Video Game Library for PlusOS & Retro Systems
 * Provides 3D vector/matrix math, procedural 3D model generation (cubes, hexagonal
 * prisms, pyramids, octahedrons, spheres, cylinders, toruses, spacecraft, cars),
 * 1-bit 16-shade Bayer ordered dithering, lighting, rasterization (wireframe &
 * solid shaded), camera systems, collision detection, and entity logic.
 */
#ifndef PLUSOS_3D_H
#define PLUSOS_3D_H

#include <stdint.h>
#include <stdbool.h>
#include <plusos/fastmath.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH  512
#define SCREEN_HEIGHT 342

/* ============================================================================
 * 0. Fixed-point core (16.16 signed)
 * ----------------------------------------------------------------------------
 * The Macintosh Plus/SE 68000 has NO FPU: every C `float` op is emulated in
 * software by libgcc (hundreds of cycles per multiply, ~1000 per divide,
 * thousands per sinf/sqrtf). The entire per-frame 3D pipeline therefore uses
 * 16.16 fixed-point integers, which map to the 68000's hardware MULS/DIVS.
 * `float` survives only at API boundaries (one-time setup, mesh building),
 * where it is converted to fixed once with FIXF(). Never use float per vertex.
 * ============================================================================ */

typedef int32_t fix_t;                         /* 16.16 signed fixed-point */
#define FIX_SHIFT 16
#define FIX_ONE   ((fix_t)1 << FIX_SHIFT)
#define FIX_HALF  ((fix_t)1 << (FIX_SHIFT - 1))
#define FIXF(f)   ((fix_t)((f) * 65536.0f + ((f) < 0 ? -0.5f : 0.5f)))  /* float -> fixed */
#define FIXI(i)   ((fix_t)(i) << FIX_SHIFT)                              /* int   -> fixed */
#define FIX_INT(x) ((int)((x) >> FIX_SHIFT))                            /* fixed -> int (floor) */

/* 16.16 multiply from four 16x16 MULS/MULU instructions - NOT int64, which on
 * the 68000 is a slow libgcc routine (see plusos/fastmath.h). */
static inline __attribute__((always_inline)) fix_t fix_mul(fix_t a, fix_t b) {
    return fix_mul_fast(a, b);
}
/* 16.16 divide. Uses a 64-bit library divide: slow - setup code only, never
 * per vertex/pixel (the renderer uses DIVS.W via divs16 instead). */
static inline fix_t fix_div(fix_t a, fix_t b) {
    return b ? (fix_t)(((int64_t)a << FIX_SHIFT) / (int64_t)b) : 0;
}

/* ============================================================================
 * 1. 3D Math Types & Structures
 * ============================================================================ */

typedef struct {
    fix_t x, y, z;      /* 16.16 fixed-point */
} Vec3;

typedef struct {
    fix_t x, y;         /* 16.16 fixed-point */
} Vec2;

typedef struct {
    fix_t m[4][4];      /* 16.16 fixed-point */
} Mat4;

typedef struct {
    int v[3];           /* Vertex indices */
    Vec3 normal;        /* Face normal */
    fix_t shade;        /* Light intensity (0..FIX_ONE) */
    uint8_t pattern;    /* Dither pattern index (0 to 15) */
    fix_t avg_z;        /* For Painter's depth sorting */
} Face3D;

typedef struct {
    Vec3 *vertices;
    Vec3 *transformed;  /* World-transformed vertices */
    Vec2 *projected;    /* Screen-projected vertices (2D) */
    Face3D *faces;
    int num_vertices;
    int num_faces;
    int max_vertices;
    int max_faces;
} Mesh3D;

/* 3D Ray for collision & raycasting */
typedef struct {
    Vec3 origin;
    Vec3 dir;
} Ray3D;

/* 3D Axis-Aligned Bounding Box */
typedef struct {
    Vec3 min;
    Vec3 max;
} BoundingBox3D;

/* 2D Axis-Aligned Bounding Box */
typedef struct {
    float x, y, w, h;
} Rect2D;

/* Camera Structure */
typedef struct {
    Vec3 pos;
    Vec3 target;
    Vec3 up;
    float yaw;
    float pitch;
    float fov;
    Mat4 view_mat;
    Mat4 proj_mat;
} Camera3D;

/* Render Mode Flags */
enum {
    RENDER_WIREFRAME   = 1,
    RENDER_SOLID_FLAT  = 2,
    RENDER_POINTS      = 4,
    RENDER_NORMALS     = 8,
    RENDER_BACKFACE_ON = 16
};

/* ============================================================================
 * 2. Vector & Matrix Operations
 * ============================================================================ */

/* vec3()/vec2() accept float literals (as all existing call sites use) and
 * convert to fixed once. Use them for setup/mesh data, never per-vertex. */
static inline Vec3 vec3(float x, float y, float z) {
    Vec3 v = { FIXF(x), FIXF(y), FIXF(z) };
    return v;
}

static inline Vec2 vec2(float x, float y) {
    Vec2 v = { FIXF(x), FIXF(y) };
    return v;
}

/* Build a vector straight from fixed-point components (no float conversion). */
static inline Vec3 vec3f(fix_t x, fix_t y, fix_t z) {
    Vec3 v = { x, y, z };
    return v;
}

Vec3  vec3_add(Vec3 a, Vec3 b);
Vec3  vec3_sub(Vec3 a, Vec3 b);
Vec3  vec3_scale(Vec3 v, float s);
fix_t vec3_dot(Vec3 a, Vec3 b);
Vec3  vec3_cross(Vec3 a, Vec3 b);
fix_t vec3_length(Vec3 v);
Vec3  vec3_normalize(Vec3 v);
Vec3  vec3_lerp(Vec3 a, Vec3 b, float t);

Mat4  mat4_identity(void);
Mat4  mat4_translate(float tx, float ty, float tz);
Mat4  mat4_rotate_x(float rad);
Mat4  mat4_rotate_y(float rad);
Mat4  mat4_rotate_z(float rad);
Mat4  mat4_scale(float sx, float sy, float sz);
Mat4  mat4_perspective(float fov_rad, float aspect, float near_z, float far_z);
Mat4  mat4_lookat(Vec3 eye, Vec3 target, Vec3 up);
Mat4  mat4_mul(Mat4 a, Mat4 b);
Vec3  mat4_mul_vec3(Mat4 m, Vec3 v);

/* ============================================================================
 * 3. Procedural 3D Model Generators
 * ============================================================================ */

Mesh3D *mesh_create(int max_vertices, int max_faces);
void    mesh_free(Mesh3D *m);
void    mesh_compute_normals(Mesh3D *m);

/* Procedural Primitive Builders */
Mesh3D *mesh_build_cube(float size);
Mesh3D *mesh_build_hex_prism(float radius, float height);
Mesh3D *mesh_build_pyramid(float base_width, float height);
Mesh3D *mesh_build_octahedron(float radius);
Mesh3D *mesh_build_cylinder(float radius, float height, int segments);
Mesh3D *mesh_build_sphere(float radius, int rings, int sectors);
Mesh3D *mesh_build_torus(float r_torus, float r_tube, int rings, int sides);
Mesh3D *mesh_build_plane(float width, float depth, int grid_x, int grid_z);

/* Procedural Video Game Models */
Mesh3D *mesh_build_car(float scale);
Mesh3D *mesh_build_spaceship(float scale);
Mesh3D *mesh_build_asteroid(float radius, int subdivisions, int seed);
Mesh3D *mesh_build_track_segment(float length, float width, float bank_angle);

/* ============================================================================
 * 4. 1-Bit Textures & Bayer Dithering
 * ============================================================================ */

/* 16 Bayer Ordered Dither Patterns (4x4) representing grayscale shades from
 * 0 (100% White) to 15 (100% Solid Black). Essential for 1-bit classic Mac rendering! */
extern const uint8_t BAYER_PATTERNS[16][8];

/* Texture generator functions */
uint8_t get_dither_pattern_for_shade(float light_intensity);
void    generate_checkerboard_texture(uint8_t *buffer, int w, int h, int tile_size);
void    generate_grid_texture(uint8_t *buffer, int w, int h, int grid_size);

/* ============================================================================
 * 5. 3D Rasterizer & Rendering Pipeline
 * ============================================================================ */

void renderer_init(void);
void renderer_set_camera(Camera3D *cam);
void renderer_set_light(Vec3 dir, float ambient, float diffuse);
void renderer_draw_mesh(Mesh3D *mesh, Mat4 model_mat, int render_flags);

/* Low-level rasterization primitives */
void raster_line_3d(Vec2 p0, Vec2 p1);
void raster_triangle_wire(Vec2 p0, Vec2 p1, Vec2 p2);
void raster_triangle_solid(Vec2 p0, Vec2 p1, Vec2 p2, uint8_t dither_pattern);

/* ============================================================================
 * 6. Video Game Logic & Collision Engine
 * ============================================================================ */

/* 2D Collision */
bool check_collision_aabb2d(Rect2D a, Rect2D b);
bool check_collision_point_rect(float px, float py, Rect2D r);

/* 3D Collision */
bool check_collision_aabb3d(BoundingBox3D a, BoundingBox3D b);
bool check_collision_sphere(Vec3 center_a, float r_a, Vec3 center_b, float r_b);
bool check_collision_ray_sphere(Ray3D ray, Vec3 center, float radius, float *t_hit);
bool check_collision_ray_plane(Ray3D ray, Vec3 plane_point, Vec3 plane_normal, float *t_hit);

/* Entity Component System for 3D Video Games */
typedef struct Entity3D {
    int id;
    bool active;
    int type;
    Vec3 pos;
    Vec3 vel;
    Vec3 rot;           /* Euler angles: pitch, yaw, roll */
    Vec3 rot_vel;
    Vec3 scale;
    Mesh3D *mesh;
    BoundingBox3D bounds;
    int render_flags;
    void (*update)(struct Entity3D *self, float dt);
} Entity3D;

#define MAX_GAME_ENTITIES 128
extern Entity3D game_entities[MAX_GAME_ENTITIES];
extern int num_game_entities;

Entity3D *entity_spawn(int type, Vec3 pos, Mesh3D *mesh);
void      entity_destroy(Entity3D *e);
void      entities_update_all(float dt);
void      entities_render_all(Camera3D *cam);

/* Game Frame Timing Helper */
typedef struct {
    uint32_t last_tick;
    float dt;
    float fps;
    int frame_count;
} GameTimer;

void timer_init(GameTimer *t);
void timer_tick(GameTimer *t);

#ifdef __cplusplus
}
#endif

#endif /* PLUSOS_3D_H */
