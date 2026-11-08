/* plus3d_mesh.c - 3D Mesh Management and Procedural Generators */
#include "plus3d_internal.h"

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

