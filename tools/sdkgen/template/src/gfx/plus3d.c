/* plus3d.c - Implementation of 3D Engine & Video Game Library for PlusOS
 *
 * FIXED-POINT PIPELINE (16.16). The 68000 has no FPU, so the per-frame path
 * (matrix build, vertex transform, projection, lighting, rasterization) is all
 * integer math on the CPU's hardware MULS/DIVS. Trig is a precomputed table.
 * `float` is used only at one-time API boundaries (mesh building, perspective
 * setup) and converted to fixed with FIXF(); it never runs per vertex.
 */
#include <plus3d/plus3d.h>
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>              /* sinf/cosf/tanf: setup & table build only */

/* 16 Bayer Ordered Dither Patterns (4x4 tiled into 8-row uint8 bytes)
 * 0 = 100% White (No ink) -> 15 = 100% Solid Black */
const uint8_t BAYER_PATTERNS[16][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 0:  0%   black */
    {0x88, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00}, /* 1:  6.25% */
    {0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00}, /* 2: 12.5% */
    {0xA8, 0x00, 0x22, 0x00, 0xA8, 0x00, 0x22, 0x00}, /* 3: 18.75% */
    {0xAA, 0x00, 0x22, 0x00, 0xAA, 0x00, 0x22, 0x00}, /* 4: 25% */
    {0xAA, 0x00, 0xA2, 0x00, 0xAA, 0x00, 0xA2, 0x00}, /* 5: 31.25% */
    {0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00}, /* 6: 37.5% */
    {0xAA, 0x44, 0xAA, 0x00, 0xAA, 0x44, 0xAA, 0x00}, /* 7: 43.75% */
    {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55}, /* 8: 50% checkerboard */
    {0xEA, 0x55, 0xAA, 0x55, 0xEA, 0x55, 0xAA, 0x55}, /* 9: 56.25% */
    {0xEE, 0x55, 0xAA, 0x55, 0xEE, 0x55, 0xAA, 0x55}, /* 10: 62.5% */
    {0xEE, 0x55, 0xBA, 0x55, 0xEE, 0x55, 0xBA, 0x55}, /* 11: 68.75% */
    {0xEE, 0x55, 0xBB, 0x55, 0xEE, 0x55, 0xBB, 0x55}, /* 12: 75% */
    {0xFE, 0x55, 0xBB, 0x55, 0xFE, 0x55, 0xBB, 0x55}, /* 13: 81.25% */
    {0xFE, 0xD5, 0xBB, 0x55, 0xFE, 0xD5, 0xBB, 0x55}, /* 14: 87.5% */
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}  /* 15: 100% Solid Black */
};

/* Global lighting configuration (fixed-point) */
static Vec3  g_light_dir = { FIXF(0.577f), FIXF(-0.577f), FIXF(0.577f) };
static fix_t g_ambient = FIXF(0.25f);
static fix_t g_diffuse = FIXF(0.75f);
static Camera3D *g_camera = NULL;

Entity3D game_entities[MAX_GAME_ENTITIES];
int num_game_entities = 0;

/* ============================================================================
 * Fixed-point helpers: sqrt and trig table
 * ============================================================================ */

/* 16.16 square root. For a = A*2^16, computes sqrt(A)*2^16 via a 64-bit
 * bitwise integer sqrt of (a << 16). ~32 iterations, no floating point. */
static fix_t fix_sqrt(fix_t a) {
    if (a <= 0) return 0;
    uint64_t v = ((uint64_t)(uint32_t)a) << 16;
    uint64_t rem = 0, root = 0;
    for (int i = 0; i < 32; i++) {
        root <<= 1;
        rem = (rem << 2) | (v >> 62);
        v <<= 2;
        root++;
        if (root <= rem) { rem -= root; root++; }
        else root--;
    }
    return (fix_t)(root >> 1);
}

/* Sine table: 1024 steps over 2*pi, in 16.16. Built once from sinf() (setup
 * only - this runs a single time, not per frame). */
#define TRIG_STEPS 1024
static fix_t g_sin[TRIG_STEPS];
static int   g_trig_ready = 0;
#define TRIG_SCALE ((float)TRIG_STEPS / (2.0f * (float)M_PI))

static void ensure_trig(void) {
    if (g_trig_ready) return;
    for (int i = 0; i < TRIG_STEPS; i++)
        g_sin[i] = FIXF(sinf((float)i * (2.0f * (float)M_PI / (float)TRIG_STEPS)));
    g_trig_ready = 1;
}

/* fixed sin/cos of a float radian angle (one float mul at the boundary, then
 * a table lookup - the expensive libgcc sinf/cosf never runs per frame). */
static fix_t fsin(float rad) {
    ensure_trig();
    int idx = (int)(rad * TRIG_SCALE) & (TRIG_STEPS - 1);
    return g_sin[idx];
}
static fix_t fcos(float rad) {
    ensure_trig();
    int idx = ((int)(rad * TRIG_SCALE) + TRIG_STEPS / 4) & (TRIG_STEPS - 1);
    return g_sin[idx];
}

/* ============================================================================
 * Vector & Matrix Implementation (fixed-point)
 * ============================================================================ */

Vec3 vec3_add(Vec3 a, Vec3 b) { return vec3f(a.x + b.x, a.y + b.y, a.z + b.z); }
Vec3 vec3_sub(Vec3 a, Vec3 b) { return vec3f(a.x - b.x, a.y - b.y, a.z - b.z); }

Vec3 vec3_scale(Vec3 v, float s) {
    fix_t fs = FIXF(s);
    return vec3f(fix_mul(v.x, fs), fix_mul(v.y, fs), fix_mul(v.z, fs));
}

fix_t vec3_dot(Vec3 a, Vec3 b) {
    return fix_mul(a.x, b.x) + fix_mul(a.y, b.y) + fix_mul(a.z, b.z);
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return vec3f(
        fix_mul(a.y, b.z) - fix_mul(a.z, b.y),
        fix_mul(a.z, b.x) - fix_mul(a.x, b.z),
        fix_mul(a.x, b.y) - fix_mul(a.y, b.x)
    );
}

fix_t vec3_length(Vec3 v) {
    return fix_sqrt(vec3_dot(v, v));
}

Vec3 vec3_normalize(Vec3 v) {
    fix_t len = vec3_length(v);
    if (len > 64) {   /* ~0.001 */
        return vec3f(fix_div(v.x, len), fix_div(v.y, len), fix_div(v.z, len));
    }
    return vec3f(0, 0, 0);
}

Vec3 vec3_lerp(Vec3 a, Vec3 b, float t) {
    fix_t ft = FIXF(t);
    return vec3f(
        a.x + fix_mul(b.x - a.x, ft),
        a.y + fix_mul(b.y - a.y, ft),
        a.z + fix_mul(b.z - a.z, ft)
    );
}

Mat4 mat4_identity(void) {
    Mat4 m;
    memset(&m, 0, sizeof(m));
    m.m[0][0] = FIX_ONE;
    m.m[1][1] = FIX_ONE;
    m.m[2][2] = FIX_ONE;
    m.m[3][3] = FIX_ONE;
    return m;
}

Mat4 mat4_translate(float tx, float ty, float tz) {
    Mat4 m = mat4_identity();
    m.m[3][0] = FIXF(tx);
    m.m[3][1] = FIXF(ty);
    m.m[3][2] = FIXF(tz);
    return m;
}

/* internal: translate/scale straight from fixed components (for the ECS) */
static Mat4 mat4_translate_fix(fix_t x, fix_t y, fix_t z) {
    Mat4 m = mat4_identity();
    m.m[3][0] = x; m.m[3][1] = y; m.m[3][2] = z;
    return m;
}
static Mat4 mat4_scale_fix(fix_t x, fix_t y, fix_t z) {
    Mat4 m = mat4_identity();
    m.m[0][0] = x; m.m[1][1] = y; m.m[2][2] = z;
    return m;
}

Mat4 mat4_rotate_x(float rad) {
    Mat4 m = mat4_identity();
    fix_t c = fcos(rad), s = fsin(rad);
    m.m[1][1] = c;
    m.m[1][2] = s;
    m.m[2][1] = -s;
    m.m[2][2] = c;
    return m;
}

Mat4 mat4_rotate_y(float rad) {
    Mat4 m = mat4_identity();
    fix_t c = fcos(rad), s = fsin(rad);
    m.m[0][0] = c;
    m.m[0][2] = -s;
    m.m[2][0] = s;
    m.m[2][2] = c;
    return m;
}

Mat4 mat4_rotate_z(float rad) {
    Mat4 m = mat4_identity();
    fix_t c = fcos(rad), s = fsin(rad);
    m.m[0][0] = c;
    m.m[0][1] = s;
    m.m[1][0] = -s;
    m.m[1][1] = c;
    return m;
}

Mat4 mat4_scale(float sx, float sy, float sz) {
    return mat4_scale_fix(FIXF(sx), FIXF(sy), FIXF(sz));
}

/* Row-by-row: r[i] = sum_k a[i][k] * b[k]. Rotation/translation/projection
 * matrices are mostly 0s and 1s, so those terms skip the multiply entirely. */
Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 r;
    memset(&r, 0, sizeof(r));
    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 4; k++) {
            fix_t s = a.m[i][k];
            if (s == 0) continue;
            if (s == FIX_ONE) {
                r.m[i][0] += b.m[k][0]; r.m[i][1] += b.m[k][1];
                r.m[i][2] += b.m[k][2]; r.m[i][3] += b.m[k][3];
            } else {
                r.m[i][0] += fix_mul(s, b.m[k][0]); r.m[i][1] += fix_mul(s, b.m[k][1]);
                r.m[i][2] += fix_mul(s, b.m[k][2]); r.m[i][3] += fix_mul(s, b.m[k][3]);
            }
        }
    }
    return r;
}

Vec3 mat4_mul_vec3(Mat4 m, Vec3 v) {
    Vec3 r;
    r.x = fix_mul(v.x, m.m[0][0]) + fix_mul(v.y, m.m[1][0]) + fix_mul(v.z, m.m[2][0]) + m.m[3][0];
    r.y = fix_mul(v.x, m.m[0][1]) + fix_mul(v.y, m.m[1][1]) + fix_mul(v.z, m.m[2][1]) + m.m[3][1];
    r.z = fix_mul(v.x, m.m[0][2]) + fix_mul(v.y, m.m[1][2]) + fix_mul(v.z, m.m[2][2]) + m.m[3][2];
    fix_t w = fix_mul(v.x, m.m[0][3]) + fix_mul(v.y, m.m[1][3]) + fix_mul(v.z, m.m[2][3]) + m.m[3][3];
    if (w != FIX_ONE && (w > 64 || w < -64)) {
        r.x = fix_div(r.x, w);
        r.y = fix_div(r.y, w);
        r.z = fix_div(r.z, w);
    }
    return r;
}

Mat4 mat4_perspective(float fov_rad, float aspect, float near_z, float far_z) {
    /* one-time setup: float math here, stored as fixed */
    Mat4 m;
    memset(&m, 0, sizeof(m));
    float tan_half = tanf(fov_rad * 0.5f);
    m.m[0][0] = FIXF(1.0f / (aspect * tan_half));
    m.m[1][1] = FIXF(1.0f / tan_half);
    m.m[2][2] = FIXF(-(far_z + near_z) / (far_z - near_z));
    m.m[2][3] = FIXF(-1.0f);
    m.m[3][2] = FIXF(-(2.0f * far_z * near_z) / (far_z - near_z));
    return m;
}

Mat4 mat4_lookat(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = vec3_normalize(vec3_sub(target, eye));
    Vec3 s = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);

    Mat4 m = mat4_identity();
    m.m[0][0] = s.x;  m.m[1][0] = s.y;  m.m[2][0] = s.z;
    m.m[0][1] = u.x;  m.m[1][1] = u.y;  m.m[2][1] = u.z;
    m.m[0][2] = -f.x; m.m[1][2] = -f.y; m.m[2][2] = -f.z;
    m.m[3][0] = -vec3_dot(s, eye);
    m.m[3][1] = -vec3_dot(u, eye);
    m.m[3][2] =  vec3_dot(f, eye);
    return m;
}

/* ============================================================================
 * Mesh Management & Procedural Models
 *   (builders use float locals for one-time geometry; vec3() converts to fixed)
 * ============================================================================ */

Mesh3D *mesh_create(int max_vertices, int max_faces) {
    Mesh3D *m = (Mesh3D *)malloc(sizeof(Mesh3D));
    if (!m) return NULL;
    m->vertices    = (Vec3 *)malloc(sizeof(Vec3) * max_vertices);
    m->transformed = (Vec3 *)malloc(sizeof(Vec3) * max_vertices);
    m->projected   = (Vec2 *)malloc(sizeof(Vec2) * max_vertices);
    m->faces       = (Face3D *)malloc(sizeof(Face3D) * max_faces);
    m->num_vertices = 0;
    m->num_faces    = 0;
    m->max_vertices = max_vertices;
    m->max_faces    = max_faces;
    return m;
}

void mesh_free(Mesh3D *m) {
    if (!m) return;
    free(m->vertices);
    free(m->transformed);
    free(m->projected);
    free(m->faces);
    free(m);
}

void mesh_compute_normals(Mesh3D *m) {
    for (int i = 0; i < m->num_faces; i++) {
        Face3D *f = &m->faces[i];
        Vec3 v0 = m->vertices[f->v[0]];
        Vec3 v1 = m->vertices[f->v[1]];
        Vec3 v2 = m->vertices[f->v[2]];
        Vec3 edge1 = vec3_sub(v1, v0);
        Vec3 edge2 = vec3_sub(v2, v0);
        f->normal = vec3_normalize(vec3_cross(edge1, edge2));
    }
}

Mesh3D *mesh_build_cube(float s) {
    float h = s * 0.5f;
    Mesh3D *m = mesh_create(8, 12);
    if (!m) return NULL;
    m->num_vertices = 8;
    m->num_faces = 12;

    m->vertices[0] = vec3(-h, -h, -h);
    m->vertices[1] = vec3( h, -h, -h);
    m->vertices[2] = vec3( h,  h, -h);
    m->vertices[3] = vec3(-h,  h, -h);
    m->vertices[4] = vec3(-h, -h,  h);
    m->vertices[5] = vec3( h, -h,  h);
    m->vertices[6] = vec3( h,  h,  h);
    m->vertices[7] = vec3(-h,  h,  h);

    int faces[12][3] = {
        {0, 2, 1}, {0, 3, 2}, /* Front */
        {5, 6, 4}, {4, 6, 7}, /* Back */
        {4, 7, 0}, {0, 7, 3}, /* Left */
        {1, 2, 5}, {5, 2, 6}, /* Right */
        {3, 7, 2}, {2, 7, 6}, /* Top */
        {4, 0, 5}, {5, 0, 1}  /* Bottom */
    };

    for (int i = 0; i < 12; i++) {
        m->faces[i].v[0] = faces[i][0];
        m->faces[i].v[1] = faces[i][1];
        m->faces[i].v[2] = faces[i][2];
    }
    mesh_compute_normals(m);
    return m;
}

Mesh3D *mesh_build_cylinder(float radius, float height, int segments) {
    if (segments < 3) segments = 3;
    if (segments > 24) segments = 24;
    int max_v = segments * 2 + 2;
    int max_f = segments * 4;
    Mesh3D *m = mesh_create(max_v, max_f);
    if (!m) return NULL;
    float hh = height * 0.5f;
    for (int i = 0; i < segments; i++) {
        float ang = 2.0f * (float)M_PI * (float)i / (float)segments;
        float x = radius * cosf(ang);
        float z = radius * sinf(ang);
        m->vertices[i]            = vec3(x,  hh, z); /* top ring */
        m->vertices[segments + i] = vec3(x, -hh, z); /* bottom ring */
    }
    int top_c = segments * 2;
    int bot_c = segments * 2 + 1;
    m->vertices[top_c] = vec3(0.0f,  hh, 0.0f);
    m->vertices[bot_c] = vec3(0.0f, -hh, 0.0f);
    m->num_vertices = max_v;

    int nf = 0;
    for (int i = 0; i < segments; i++) {
        int next = (i + 1) % segments;
        /* Top cap */
        m->faces[nf].v[0] = top_c; m->faces[nf].v[1] = next; m->faces[nf].v[2] = i; nf++;
        /* Bottom cap */
        m->faces[nf].v[0] = bot_c; m->faces[nf].v[1] = segments + i; m->faces[nf].v[2] = segments + next; nf++;
        /* Side quad (2 tris) */
        m->faces[nf].v[0] = i; m->faces[nf].v[1] = next; m->faces[nf].v[2] = segments + next; nf++;
        m->faces[nf].v[0] = i; m->faces[nf].v[1] = segments + next; m->faces[nf].v[2] = segments + i; nf++;
    }
    m->num_faces = nf;
    mesh_compute_normals(m);
    return m;
}

Mesh3D *mesh_build_hex_prism(float radius, float height) {
    return mesh_build_cylinder(radius, height, 6);
}

Mesh3D *mesh_build_octahedron(float r) {
    Mesh3D *m = mesh_create(6, 8);
    if (!m) return NULL;
    m->num_vertices = 6;
    m->num_faces = 8;
    m->vertices[0] = vec3( 0,  r,  0); /* Top */
    m->vertices[1] = vec3( 0, -r,  0); /* Bottom */
    m->vertices[2] = vec3(-r,  0,  0); /* Left */
    m->vertices[3] = vec3( r,  0,  0); /* Right */
    m->vertices[4] = vec3( 0,  0,  r); /* Front */
    m->vertices[5] = vec3( 0,  0, -r); /* Back */

    int faces[8][3] = {
        {0, 4, 3}, {0, 3, 5}, {0, 5, 2}, {0, 2, 4},
        {1, 3, 4}, {1, 5, 3}, {1, 2, 5}, {1, 4, 2}
    };
    for (int i = 0; i < 8; i++) {
        m->faces[i].v[0] = faces[i][0];
        m->faces[i].v[1] = faces[i][1];
        m->faces[i].v[2] = faces[i][2];
    }
    mesh_compute_normals(m);
    return m;
}

Mesh3D *mesh_build_pyramid(float base_w, float h) {
    float b = base_w * 0.5f;
    Mesh3D *m = mesh_create(5, 6);
    if (!m) return NULL;
    m->num_vertices = 5;
    m->num_faces = 6;

    m->vertices[0] = vec3(-b, -h * 0.5f, -b);
    m->vertices[1] = vec3( b, -h * 0.5f, -b);
    m->vertices[2] = vec3( b, -h * 0.5f,  b);
    m->vertices[3] = vec3(-b, -h * 0.5f,  b);
    m->vertices[4] = vec3( 0,  h * 0.5f,  0); /* Apex */

    int faces[6][3] = {
        {0, 2, 1}, {0, 3, 2}, /* Base */
        {0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {3, 0, 4} /* Sides */
    };
    for (int i = 0; i < 6; i++) {
        m->faces[i].v[0] = faces[i][0];
        m->faces[i].v[1] = faces[i][1];
        m->faces[i].v[2] = faces[i][2];
    }
    mesh_compute_normals(m);
    return m;
}

Mesh3D *mesh_build_sphere(float r, int rings, int sectors) {
    if (rings < 3) rings = 3;
    if (sectors < 4) sectors = 4;
    int max_v = rings * sectors;
    int max_f = (rings - 1) * sectors * 2;
    Mesh3D *m = mesh_create(max_v, max_f);
    if (!m) return NULL;
    m->num_vertices = 0;
    m->num_faces = 0;

    for (int i = 0; i < rings; i++) {
        float phi = (float)M_PI * (float)i / (float)(rings - 1);
        for (int j = 0; j < sectors; j++) {
            float theta = 2.0f * (float)M_PI * (float)j / (float)sectors;
            float x = r * sinf(phi) * cosf(theta);
            float y = r * cosf(phi);
            float z = r * sinf(phi) * sinf(theta);
            m->vertices[m->num_vertices++] = vec3(x, y, z);
        }
    }

    for (int i = 0; i < rings - 1; i++) {
        for (int j = 0; j < sectors; j++) {
            int next_j = (j + 1) % sectors;
            int i0 = i * sectors + j;
            int i1 = (i + 1) * sectors + j;
            int i2 = (i + 1) * sectors + next_j;
            int i3 = i * sectors + next_j;

            m->faces[m->num_faces].v[0] = i0;
            m->faces[m->num_faces].v[1] = i1;
            m->faces[m->num_faces].v[2] = i2;
            m->num_faces++;

            m->faces[m->num_faces].v[0] = i0;
            m->faces[m->num_faces].v[1] = i2;
            m->faces[m->num_faces].v[2] = i3;
            m->num_faces++;
        }
    }
    mesh_compute_normals(m);
    return m;
}

Mesh3D *mesh_build_torus(float r_torus, float r_tube, int rings, int sides) {
    if (rings < 4) rings = 4;
    if (sides < 4) sides = 4;
    Mesh3D *m = mesh_create(rings * sides, rings * sides * 2);
    if (!m) return NULL;
    m->num_vertices = 0;
    m->num_faces = 0;
    for (int i = 0; i < rings; i++) {
        float u = 2.0f * (float)M_PI * (float)i / (float)rings;
        float cu = cosf(u), su = sinf(u);
        for (int j = 0; j < sides; j++) {
            float v = 2.0f * (float)M_PI * (float)j / (float)sides;
            float cv = cosf(v), sv = sinf(v);
            float x = (r_torus + r_tube * cv) * cu;
            float y = r_tube * sv;
            float z = (r_torus + r_tube * cv) * su;
            m->vertices[m->num_vertices++] = vec3(x, y, z);
        }
    }
    for (int i = 0; i < rings; i++) {
        int ni = (i + 1) % rings;
        for (int j = 0; j < sides; j++) {
            int nj = (j + 1) % sides;
            int v0 = i * sides + j, v1 = ni * sides + j;
            int v2 = ni * sides + nj, v3 = i * sides + nj;
            m->faces[m->num_faces].v[0] = v0; m->faces[m->num_faces].v[1] = v1; m->faces[m->num_faces].v[2] = v2; m->num_faces++;
            m->faces[m->num_faces].v[0] = v0; m->faces[m->num_faces].v[1] = v2; m->faces[m->num_faces].v[2] = v3; m->num_faces++;
        }
    }
    mesh_compute_normals(m);
    return m;
}

Mesh3D *mesh_build_plane(float width, float depth, int gx, int gz) {
    if (gx < 1) gx = 1;
    if (gz < 1) gz = 1;
    Mesh3D *m = mesh_create((gx + 1) * (gz + 1), gx * gz * 2);
    if (!m) return NULL;
    m->num_vertices = 0;
    m->num_faces = 0;
    for (int z = 0; z <= gz; z++) {
        float fz = ((float)z / (float)gz - 0.5f) * depth;
        for (int x = 0; x <= gx; x++) {
            float fx = ((float)x / (float)gx - 0.5f) * width;
            m->vertices[m->num_vertices++] = vec3(fx, 0.0f, fz);
        }
    }
    for (int z = 0; z < gz; z++) {
        for (int x = 0; x < gx; x++) {
            int v0 = z * (gx + 1) + x, v1 = v0 + 1;
            int v2 = (z + 1) * (gx + 1) + x, v3 = v2 + 1;
            m->faces[m->num_faces].v[0] = v0; m->faces[m->num_faces].v[1] = v2; m->faces[m->num_faces].v[2] = v1; m->num_faces++;
            m->faces[m->num_faces].v[0] = v1; m->faces[m->num_faces].v[1] = v2; m->faces[m->num_faces].v[2] = v3; m->num_faces++;
        }
    }
    mesh_compute_normals(m);
    return m;
}

Mesh3D *mesh_build_asteroid(float radius, int subdivisions, int seed) {
    (void)subdivisions;
    Mesh3D *m = mesh_build_sphere(radius, 5, 7);
    if (!m) return NULL;
    uint32_t s = (uint32_t)(seed ? seed : 1337);
    for (int i = 0; i < m->num_vertices; i++) {
        s = s * 1103515245u + 12345u;
        float jitter = 0.8f + (float)((s >> 16) & 0xFF) / 640.0f;
        m->vertices[i] = vec3_scale(m->vertices[i], jitter);
    }
    mesh_compute_normals(m);
    return m;
}

Mesh3D *mesh_build_track_segment(float length, float width, float bank_angle) {
    Mesh3D *m = mesh_build_plane(width, length, 2, 2);
    if (!m) return NULL;
    if (bank_angle != 0.0f) {
        Mat4 rot = mat4_rotate_z(bank_angle);
        for (int i = 0; i < m->num_vertices; i++)
            m->vertices[i] = mat4_mul_vec3(rot, m->vertices[i]);
        mesh_compute_normals(m);
    }
    return m;
}

Mesh3D *mesh_build_spaceship(float s) {
    Mesh3D *m = mesh_create(8, 12);
    if (!m) return NULL;
    m->num_vertices = 8;
    m->num_faces = 12;

    /* Sleek wedge fighter */
    m->vertices[0] = vec3( 0.0f,  0.0f,  s * 2.0f);  /* Nose */
    m->vertices[1] = vec3(-s,     0.0f, -s);         /* Left Wing */
    m->vertices[2] = vec3( s,     0.0f, -s);         /* Right Wing */
    m->vertices[3] = vec3( 0.0f,  s * 0.5f, -s * 0.8f); /* Cockpit top */
    m->vertices[4] = vec3( 0.0f, -s * 0.3f, -s * 0.8f); /* Belly */
    m->vertices[5] = vec3(-s * 0.4f, s * 0.6f, -s * 1.2f); /* Left Fin */
    m->vertices[6] = vec3( s * 0.4f, s * 0.6f, -s * 1.2f); /* Right Fin */
    m->vertices[7] = vec3( 0.0f, 0.0f, -s * 1.2f);    /* Thruster exhaust */

    int f[12][3] = {
        {0, 3, 1}, {0, 2, 3}, /* Top hull */
        {0, 1, 4}, {0, 4, 2}, /* Bottom hull */
        {1, 3, 5}, {2, 6, 3}, /* Fins */
        {1, 5, 7}, {2, 7, 6}, /* Fin backs */
        {3, 6, 7}, {3, 7, 5}, /* Engine top */
        {4, 7, 1}, {4, 2, 7}  /* Engine bottom */
    };
    for (int i = 0; i < 12; i++) {
        m->faces[i].v[0] = f[i][0];
        m->faces[i].v[1] = f[i][1];
        m->faces[i].v[2] = f[i][2];
    }
    mesh_compute_normals(m);
    return m;
}

Mesh3D *mesh_build_car(float s) {
    Mesh3D *m = mesh_create(12, 16);
    if (!m) return NULL;
    m->num_vertices = 12;
    m->num_faces = 16;

    float w = s * 0.6f;
    float l = s * 1.2f;
    float h = s * 0.35f;

    /* Chassis */
    m->vertices[0]  = vec3(-w, 0,  l);
    m->vertices[1]  = vec3( w, 0,  l);
    m->vertices[2]  = vec3( w, 0, -l);
    m->vertices[3]  = vec3(-w, 0, -l);
    m->vertices[4]  = vec3(-w, h,  l * 0.6f); /* Hood */
    m->vertices[5]  = vec3( w, h,  l * 0.6f);
    m->vertices[6]  = vec3(-w * 0.8f, h * 1.8f,  0); /* Roof */
    m->vertices[7]  = vec3( w * 0.8f, h * 1.8f,  0);
    m->vertices[8]  = vec3(-w * 0.8f, h * 1.8f, -l * 0.4f);
    m->vertices[9]  = vec3( w * 0.8f, h * 1.8f, -l * 0.4f);
    m->vertices[10] = vec3(-w, h, -l * 0.9f); /* Trunk */
    m->vertices[11] = vec3( w, h, -l * 0.9f);

    int f[16][3] = {
        {0, 1, 5}, {0, 5, 4}, /* Hood */
        {4, 5, 7}, {4, 7, 6}, /* Windshield */
        {6, 7, 9}, {6, 9, 8}, /* Roof */
        {8, 9, 11}, {8, 11, 10}, /* Rear window */
        {0, 4, 3}, {4, 10, 3}, /* Left side */
        {1, 2, 5}, {5, 2, 11}, /* Right side */
        {3, 10, 2}, {10, 11, 2}, /* Rear bumper */
        {0, 1, 2}, {0, 2, 3}  /* Underbody */
    };
    for (int i = 0; i < 16; i++) {
        m->faces[i].v[0] = f[i][0];
        m->faces[i].v[1] = f[i][1];
        m->faces[i].v[2] = f[i][2];
    }
    mesh_compute_normals(m);
    return m;
}

/* light 0..FIX_ONE -> Bayer index 0..15 */
static uint8_t shade_to_pattern(fix_t light) {
    if (light < 0) light = 0;
    if (light > FIX_ONE) light = FIX_ONE;
    int idx = FIX_INT(fix_mul(light, FIXI(15)) + FIX_HALF);
    if (idx < 0) idx = 0;
    if (idx > 15) idx = 15;
    return (uint8_t)idx;
}

uint8_t get_dither_pattern_for_shade(float light) {
    return shade_to_pattern(FIXF(light));
}

void generate_checkerboard_texture(uint8_t *buffer, int w, int h, int tile_size) {
    if (!buffer || tile_size <= 0) return;
    int rb = (w + 7) >> 3;
    memset(buffer, 0, (size_t)(rb * h));
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (((x / tile_size) + (y / tile_size)) & 1)
                buffer[y * rb + (x >> 3)] |= (uint8_t)(0x80 >> (x & 7));
        }
    }
}

void generate_grid_texture(uint8_t *buffer, int w, int h, int grid_size) {
    if (!buffer || grid_size <= 0) return;
    int rb = (w + 7) >> 3;
    memset(buffer, 0, (size_t)(rb * h));
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if ((x % grid_size) == 0 || (y % grid_size) == 0)
                buffer[y * rb + (x >> 3)] |= (uint8_t)(0x80 >> (x & 7));
        }
    }
}

/* ============================================================================
 * 3D Renderer & Rasterizer (integer)
 * ============================================================================ */

void renderer_init(void) {
    ensure_trig();
    g_light_dir = vec3_normalize(vec3(0.5f, 0.8f, -0.6f));
    g_ambient = FIXF(0.2f);
    g_diffuse = FIXF(0.8f);
}

void renderer_set_camera(Camera3D *cam) {
    g_camera = cam;
}

void renderer_set_light(Vec3 dir, float ambient, float diffuse) {
    g_light_dir = vec3_normalize(dir);
    g_ambient = FIXF(ambient);
    g_diffuse = FIXF(diffuse);
}

/* ----------------------------------------------------------------------------
 * Direct-to-framebuffer rasterizer.
 * Writes straight into the current PlusGFX target (normally the back buffer)
 * with pointer stepping: no function call per pixel/scanline, no multiply per
 * row (row pointer is advanced by rowbytes), and no division per scanline
 * (triangle edges step with 16.16 increments computed once per edge).
 * ---------------------------------------------------------------------------- */
extern void gfx__fill_bytes(uint8_t *p, uint32_t n, uint32_t pat32);   /* gfx_asm.S */

typedef struct {
    uint8_t *bits;
    int rb;                      /* rowbytes */
    int cl, ct, cr, cb;          /* clip rect: [cl,cr) x [ct,cb) */
} RTarget;
static RTarget rt;

static void rt_begin(void) {
    GSurface *s = gfx_get_target();
    GRect c;
    gfx_get_clip(&c);
    rt.bits = s->bits;
    rt.rb = s->rowbytes;
    rt.cl = c.l; rt.ct = c.t; rt.cr = c.r; rt.cb = c.b;
}

static inline int16_t sat16(int32_t v) {
    return v > 32767 ? 32767 : (v < -32768 ? -32768 : (int16_t)v);
}

static const uint8_t k_lmask[8] = { 0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };
static const uint8_t k_rmask[8] = { 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF };

/* Fill pixels l..r (inclusive, already clipped) of one row with `pat`. */
static inline void rt_span(uint8_t *row, int l, int r, uint8_t pat) {
    int lb = l >> 3, rbyte = r >> 3;
    uint8_t lm = k_lmask[l & 7], rm = k_rmask[r & 7];
    uint8_t *p = row + lb;
    if (lb == rbyte) {
        uint8_t mk = (uint8_t)(lm & rm);
        *p = (uint8_t)((*p & ~mk) | (pat & mk));
        return;
    }
    *p = (uint8_t)((*p & ~lm) | (pat & lm));
    p++;
    int n = rbyte - lb - 1;
    if (n > 0) {
        /* The interior is the bulk of every solid triangle: hand it to the
         * 8x-unrolled assembly filler (gfx_asm.S) rather than a C longword loop
         * that pays a dbra per 4 bytes. */
        uint16_t w16 = (uint16_t)(pat | (pat << 8));
        gfx__fill_bytes(p, (uint32_t)n, ((uint32_t)w16 << 16) | w16);
    }
    row[rbyte] = (uint8_t)((row[rbyte] & ~rm) | (pat & rm));
}

/* Flat dithered triangle, integer screen coords. */
static void rt_tri(int x0, int y0, int x1, int y1, int x2, int y2, const uint8_t *pat) {
    int t;
    if (y0 > y1) { t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
    if (y1 > y2) { t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
    if (y0 > y1) { t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
    if (y2 < rt.ct || y0 >= rt.cb || y0 == y2) return;

    /* edge slopes in 16.16: three divides per triangle, none per scanline */
    int32_t dl = ((int32_t)(x2 - x0) << 16) / (y2 - y0);
    int32_t da = (y1 > y0) ? ((int32_t)(x1 - x0) << 16) / (y1 - y0) : 0;
    int32_t db = (y2 > y1) ? ((int32_t)(x2 - x1) << 16) / (y2 - y1) : 0;

    /* edge x positions at the first visible row (rows above the clip are
     * skipped arithmetically, never walked). skip <= edge height, so
     * slope*skip <= dx<<16 and cannot overflow. */
    int y = y0 < rt.ct ? rt.ct : y0;
    int32_t xl = ((int32_t)x0 << 16) + 0x8000 + dl * (y - y0);    /* long edge x0 -> x2 */
    int32_t xs = (y < y1)                                           /* short edges */
               ? ((int32_t)x0 << 16) + 0x8000 + da * (y - y0)
               : ((int32_t)x1 << 16) + 0x8000 + db * (y - y1);

    uint8_t *row = rt.bits + ROWOFF(y, rt.rb);
    int rb = rt.rb, cl = rt.cl, cr1 = rt.cr - 1;
    int yend = y2 < rt.cb ? y2 : rt.cb - 1;

    for (; y <= yend; y++, row += rb) {
        if (y == y1) xs = ((int32_t)x1 << 16) + 0x8000;
        int a = (int)(xl >> 16), b = (int)(xs >> 16);
        if (a > b) { t = a; a = b; b = t; }
        if (a < cl) a = cl;
        if (b > cr1) b = cr1;
        if (a <= b) rt_span(row, a, b, pat[y & 7]);
        xl += dl;
        xs += (y < y1) ? da : db;
    }
}

/* 1-pixel black line, Bresenham with pointer + bit-mask stepping. */
static void rt_line(int x0, int y0, int x1, int y1) {
    /* trivial reject: both endpoints off the same side */
    if ((x0 < rt.cl && x1 < rt.cl) || (x0 >= rt.cr && x1 >= rt.cr) ||
        (y0 < rt.ct && y1 < rt.ct) || (y0 >= rt.cb && y1 >= rt.cb)) return;

    int dx = x1 - x0, dy = y1 - y0;
    int sx = 1, sgy = 1, rstep = rt.rb;
    if (dx < 0) { dx = -dx; sx = -1; }
    if (dy < 0) { dy = -dy; sgy = -1; rstep = -rstep; }
    int inside = x0 >= rt.cl && x0 < rt.cr && x1 >= rt.cl && x1 < rt.cr &&
                 y0 >= rt.ct && y0 < rt.cb && y1 >= rt.ct && y1 < rt.cb;

    uint8_t *p = rt.bits + ROWOFF(y0, rt.rb) + (x0 >> 3);
    uint8_t mk = (uint8_t)(0x80 >> (x0 & 7));
    int x = x0, y = y0;

    if (dx >= dy) {
        int err = dx >> 1;
        for (int i = 0; i <= dx; i++) {
            if (inside || (x >= rt.cl && x < rt.cr && y >= rt.ct && y < rt.cb)) *p |= mk;
            if (sx > 0) { mk >>= 1; if (!mk) { mk = 0x80; p++; } }
            else        { mk = (uint8_t)(mk << 1); if (!mk) { mk = 0x01; p--; } }
            x += sx;
            err -= dy;
            if (err < 0) { err += dx; p += rstep; y += sgy; }
        }
    } else {
        int err = dy >> 1;
        for (int i = 0; i <= dy; i++) {
            if (inside || (x >= rt.cl && x < rt.cr && y >= rt.ct && y < rt.cb)) *p |= mk;
            p += rstep; y += sgy;
            err -= dx;
            if (err < 0) {
                err += dy;
                if (sx > 0) { mk >>= 1; if (!mk) { mk = 0x80; p++; } }
                else        { mk = (uint8_t)(mk << 1); if (!mk) { mk = 0x01; p--; } }
                x += sx;
            }
        }
    }
}

void raster_line_3d(Vec2 p0, Vec2 p1) {
    rt_begin();
    rt_line(FIX_INT(p0.x), FIX_INT(p0.y), FIX_INT(p1.x), FIX_INT(p1.y));
}

void raster_triangle_wire(Vec2 p0, Vec2 p1, Vec2 p2) {
    rt_begin();
    int x0 = FIX_INT(p0.x), y0 = FIX_INT(p0.y);
    int x1 = FIX_INT(p1.x), y1 = FIX_INT(p1.y);
    int x2 = FIX_INT(p2.x), y2 = FIX_INT(p2.y);
    rt_line(x0, y0, x1, y1);
    rt_line(x1, y1, x2, y2);
    rt_line(x2, y2, x0, y0);
}

void raster_triangle_solid(Vec2 p0, Vec2 p1, Vec2 p2, uint8_t pat_idx) {
    rt_begin();
    rt_tri(FIX_INT(p0.x), FIX_INT(p0.y), FIX_INT(p1.x), FIX_INT(p1.y),
           FIX_INT(p2.x), FIX_INT(p2.y), BAYER_PATTERNS[pat_idx & 15]);
}

/* Per-frame scratch (screen coords, depth, visible-face list). */
#define R_MAXV 512
#define R_MAXF 1024
static int16_t s_sx[R_MAXV], s_sy[R_MAXV], s_w[R_MAXV];
static uint8_t s_ok[R_MAXV];
static int16_t s_fidx[R_MAXF];
static int32_t s_fkey[R_MAXF];

/* The hot path. Per vertex: 9 MULS.W + 2 DIVS.W. Per visible face: 12 MULS.W
 * for lighting (the precomputed object-space normal is rotated - no cross
 * product, sqrt or normalize per frame). No 32x32 multiplies, no 64-bit math,
 * no floats. Note: m->transformed is not updated by this renderer. */
void renderer_draw_mesh(Mesh3D *m, Mat4 model_mat, int render_flags) {
    if (!g_camera || !m) return;
    rt_begin();

    Mat4 mvp = mat4_mul(mat4_mul(model_mat, g_camera->view_mat), g_camera->proj_mat);

    /* 4.12 copies of the x, y, w columns (rows 0..2) + 8.24 translation */
    int16_t ax0 = sat16(mvp.m[0][0] >> 4), ax1 = sat16(mvp.m[1][0] >> 4), ax2 = sat16(mvp.m[2][0] >> 4);
    int16_t ay0 = sat16(mvp.m[0][1] >> 4), ay1 = sat16(mvp.m[1][1] >> 4), ay2 = sat16(mvp.m[2][1] >> 4);
    int16_t aw0 = sat16(mvp.m[0][3] >> 4), aw1 = sat16(mvp.m[1][3] >> 4), aw2 = sat16(mvp.m[2][3] >> 4);
    int32_t tx = mvp.m[3][0] << 8, ty = mvp.m[3][1] << 8, tw = mvp.m[3][3] << 8;

    int nv = m->num_vertices < R_MAXV ? m->num_vertices : R_MAXV;
    for (int i = 0; i < nv; i++) {
        const Vec3 *v = &m->vertices[i];
        int16_t vx = sat16(v->x >> 4), vy = sat16(v->y >> 4), vz = sat16(v->z >> 4);   /* 4.12 */
        int32_t X = muls16(vx, ax0) + muls16(vy, ax1) + muls16(vz, ax2) + tx;   /* 8.24 */
        int32_t Y = muls16(vx, ay0) + muls16(vy, ay1) + muls16(vz, ay2) + ty;
        int32_t W = muls16(vx, aw0) + muls16(vy, aw1) + muls16(vz, aw2) + tw;

        int32_t w12 = W >> 12;                                  /* 8.12 */
        if (w12 < 64) { s_ok[i] = 0; continue; }                /* behind the camera */
        int16_t ws = sat16(w12);
        int32_t xs = X >> 12, ys = Y >> 12;
        int32_t lim = (int32_t)ws << 4;                         /* |x/w| <= 16: DIVS stays in range */
        if (xs > lim) xs = lim; else if (xs < -lim) xs = -lim;
        if (ys > lim) ys = lim; else if (ys < -lim) ys = -lim;
        int32_t nx = (xs << 8) - xs;                            /* 255 * x (shift-sub, no MUL) */
        int32_t ny = (ys << 7) + (ys << 5) + (ys << 3) + (ys << 1);   /* 170 * y */
        int16_t sx = (int16_t)(255 + divs16(nx, ws));
        int16_t sy = (int16_t)(170 - divs16(ny, ws));
        s_sx[i] = sx; s_sy[i] = sy; s_w[i] = ws; s_ok[i] = 1;
        m->projected[i].x = (fix_t)sx << 16;
        m->projected[i].y = (fix_t)sy << 16;
    }

    /* world rotation (model upper 3x3) in 4.12, light & material in 2.14 */
    int16_t r00 = sat16(model_mat.m[0][0] >> 4), r01 = sat16(model_mat.m[0][1] >> 4), r02 = sat16(model_mat.m[0][2] >> 4);
    int16_t r10 = sat16(model_mat.m[1][0] >> 4), r11 = sat16(model_mat.m[1][1] >> 4), r12 = sat16(model_mat.m[1][2] >> 4);
    int16_t r20 = sat16(model_mat.m[2][0] >> 4), r21 = sat16(model_mat.m[2][1] >> 4), r22 = sat16(model_mat.m[2][2] >> 4);
    int16_t lx = sat16(g_light_dir.x >> 2), ly = sat16(g_light_dir.y >> 2), lz = sat16(g_light_dir.z >> 2);
    int16_t amb = sat16(g_ambient >> 2), dif = sat16(g_diffuse >> 2);

    int nf = m->num_faces < R_MAXF ? m->num_faces : R_MAXF;
    int cull = !(render_flags & RENDER_BACKFACE_ON);
    int nvis = 0;
    for (int i = 0; i < nf; i++) {
        Face3D *f = &m->faces[i];
        int a = f->v[0], b = f->v[1], c = f->v[2];
        if (a >= nv || b >= nv || c >= nv || !s_ok[a] || !s_ok[b] || !s_ok[c]) continue;

        if (cull) {
            int16_t x0 = s_sx[a], y0 = s_sy[a];
            int32_t cr = muls16((int16_t)(s_sx[b] - x0), (int16_t)(s_sy[c] - y0)) -
                         muls16((int16_t)(s_sy[b] - y0), (int16_t)(s_sx[c] - x0));
            if (cr <= 0) continue;
        }

        /* Lambert: rotate the stored unit normal into world space, dot light */
        int16_t nx = sat16(f->normal.x >> 2), ny = sat16(f->normal.y >> 2), nz = sat16(f->normal.z >> 2);
        int16_t wx = sat16((muls16(nx, r00) + muls16(ny, r10) + muls16(nz, r20)) >> 12);   /* 2.14 */
        int16_t wy = sat16((muls16(nx, r01) + muls16(ny, r11) + muls16(nz, r21)) >> 12);
        int16_t wz = sat16((muls16(nx, r02) + muls16(ny, r12) + muls16(nz, r22)) >> 12);
        int32_t d = (muls16(wx, lx) + muls16(wy, ly) + muls16(wz, lz)) >> 14;              /* 2.14 */
        if (d < 0) d = 0;
        int32_t inten = amb + (muls16(dif, sat16(d)) >> 14);
        if (inten > 16384) inten = 16384;
        if (inten < 0) inten = 0;
        f->pattern = (uint8_t)((muls16((int16_t)inten, 15) + 8192) >> 14);
        f->shade = inten << 2;

        int32_t key = (int32_t)s_w[a] + s_w[b] + s_w[c];
        f->avg_z = key;
        s_fidx[nvis] = (int16_t)i;
        s_fkey[nvis] = key;
        nvis++;
    }

    /* Painter's algorithm: insertion sort by depth, farthest (largest w) first */
    for (int i = 1; i < nvis; i++) {
        int32_t k = s_fkey[i];
        int16_t id = s_fidx[i];
        int j = i - 1;
        while (j >= 0 && s_fkey[j] < k) { s_fkey[j + 1] = s_fkey[j]; s_fidx[j + 1] = s_fidx[j]; j--; }
        s_fkey[j + 1] = k;
        s_fidx[j + 1] = id;
    }

    for (int i = 0; i < nvis; i++) {
        const Face3D *f = &m->faces[s_fidx[i]];
        int a = f->v[0], b = f->v[1], c = f->v[2];
        int x0 = s_sx[a], y0 = s_sy[a], x1 = s_sx[b], y1 = s_sy[b], x2 = s_sx[c], y2 = s_sy[c];
        if (render_flags & RENDER_SOLID_FLAT)
            rt_tri(x0, y0, x1, y1, x2, y2, BAYER_PATTERNS[f->pattern & 15]);
        if (render_flags & RENDER_WIREFRAME) {
            rt_line(x0, y0, x1, y1);
            rt_line(x1, y1, x2, y2);
            rt_line(x2, y2, x0, y0);
        }
    }
}

/* ============================================================================
 * Collision & Video Game Logic Engine (fixed-point)
 * ============================================================================ */

bool check_collision_aabb2d(Rect2D a, Rect2D b) {
    return (a.x < b.x + b.w && a.x + a.w > b.x &&
            a.y < b.y + b.h && a.y + a.h > b.y);
}

bool check_collision_point_rect(float px, float py, Rect2D r) {
    return (px >= r.x && px <= r.x + r.w && py >= r.y && py <= r.y + r.h);
}

bool check_collision_aabb3d(BoundingBox3D a, BoundingBox3D b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x &&
            a.min.y <= b.max.y && a.max.y >= b.min.y &&
            a.min.z <= b.max.z && a.max.z >= b.min.z);
}

bool check_collision_sphere(Vec3 ca, float ra, Vec3 cb, float rb) {
    fix_t dx = ca.x - cb.x, dy = ca.y - cb.y, dz = ca.z - cb.z;
    fix_t dist_sq = fix_mul(dx, dx) + fix_mul(dy, dy) + fix_mul(dz, dz);
    fix_t r_sum = FIXF(ra + rb);
    return dist_sq <= fix_mul(r_sum, r_sum);
}

bool check_collision_ray_sphere(Ray3D ray, Vec3 center, float radius, float *t_hit) {
    Vec3 oc = vec3_sub(ray.origin, center);
    fix_t a = vec3_dot(ray.dir, ray.dir);
    fix_t b = fix_mul(FIXI(2), vec3_dot(oc, ray.dir));
    fix_t rad = FIXF(radius);
    fix_t c = vec3_dot(oc, oc) - fix_mul(rad, rad);
    fix_t disc = fix_mul(b, b) - fix_mul(FIXI(4), fix_mul(a, c));
    if (disc < 0) return false;
    fix_t t = fix_div(-b - fix_sqrt(disc), fix_mul(FIXI(2), a));
    if (t > 0) {
        if (t_hit) *t_hit = (float)t / 65536.0f;
        return true;
    }
    return false;
}

bool check_collision_ray_plane(Ray3D ray, Vec3 plane_point, Vec3 plane_normal, float *t_hit) {
    fix_t denom = vec3_dot(plane_normal, ray.dir);
    if (denom > 6 || denom < -6) {   /* ~0.0001 */
        fix_t t = fix_div(vec3_dot(vec3_sub(plane_point, ray.origin), plane_normal), denom);
        if (t >= 0) {
            if (t_hit) *t_hit = (float)t / 65536.0f;
            return true;
        }
    }
    return false;
}

Entity3D *entity_spawn(int type, Vec3 pos, Mesh3D *mesh) {
    for (int i = 0; i < MAX_GAME_ENTITIES; i++) {
        if (!game_entities[i].active) {
            Entity3D *e = &game_entities[i];
            e->id = i;
            e->active = true;
            e->type = type;
            e->pos = pos;
            e->vel = vec3(0, 0, 0);
            e->rot = vec3(0, 0, 0);
            e->rot_vel = vec3(0, 0, 0);
            e->scale = vec3(1, 1, 1);
            e->mesh = mesh;
            e->render_flags = RENDER_SOLID_FLAT | RENDER_WIREFRAME;
            e->update = NULL;
            num_game_entities++;
            return e;
        }
    }
    return NULL;
}

void entity_destroy(Entity3D *e) {
    if (!e) return;
    if (e->active) {
        e->active = false;
        num_game_entities--;
    }
}

void entities_update_all(float dt) {
    for (int i = 0; i < MAX_GAME_ENTITIES; i++) {
        Entity3D *e = &game_entities[i];
        if (e->active) {
            e->pos = vec3_add(e->pos, vec3_scale(e->vel, dt));
            e->rot = vec3_add(e->rot, vec3_scale(e->rot_vel, dt));
            if (e->update) {
                e->update(e, dt);
            }
        }
    }
}

void entities_render_all(Camera3D *cam) {
    (void)cam;
    for (int i = 0; i < MAX_GAME_ENTITIES; i++) {
        Entity3D *e = &game_entities[i];
        if (e->active && e->mesh) {
            Mat4 t  = mat4_translate_fix(e->pos.x, e->pos.y, e->pos.z);
            Mat4 rx = mat4_rotate_x((float)e->rot.x / 65536.0f);
            Mat4 ry = mat4_rotate_y((float)e->rot.y / 65536.0f);
            Mat4 rz = mat4_rotate_z((float)e->rot.z / 65536.0f);
            Mat4 s  = mat4_scale_fix(e->scale.x, e->scale.y, e->scale.z);
            Mat4 model = mat4_mul(mat4_mul(mat4_mul(s, rx), mat4_mul(ry, rz)), t);
            renderer_draw_mesh(e->mesh, model, e->render_flags);
        }
    }
}

void timer_init(GameTimer *t) {
    t->last_tick = plusos->ticks();
    t->dt = 0.016f;
    t->fps = 60.0f;
    t->frame_count = 0;
}

void timer_tick(GameTimer *t) {
    uint32_t current = plusos->ticks();
    uint32_t elapsed = current - t->last_tick;
    if (elapsed == 0) elapsed = 1;
    t->dt = (float)elapsed / 60.0f;
    t->fps = 60.0f / (float)elapsed;
    t->last_tick = current;
    t->frame_count++;
}
