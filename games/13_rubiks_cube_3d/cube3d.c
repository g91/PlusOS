/* cube3d.c - 3D Rubik's Cube / Polyhedra Demo for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/OSUtil.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    float x, y, z;
} Point3D;

typedef struct {
    int v[4];
    int shade; /* 0 to 3 */
} Quad3D;

/* 8 vertices of a cube */
static const Point3D cube_verts[8] = {
    {-1.0f, -1.0f, -1.0f},
    { 1.0f, -1.0f, -1.0f},
    { 1.0f,  1.0f, -1.0f},
    {-1.0f,  1.0f, -1.0f},
    {-1.0f, -1.0f,  1.0f},
    { 1.0f, -1.0f,  1.0f},
    { 1.0f,  1.0f,  1.0f},
    {-1.0f,  1.0f,  1.0f}
};

/* 6 faces */
static const Quad3D cube_faces[6] = {
    {{0, 1, 2, 3}, 0}, /* Front */
    {{5, 4, 7, 6}, 1}, /* Back */
    {{4, 0, 3, 7}, 2}, /* Left */
    {{1, 5, 6, 2}, 3}, /* Right */
    {{3, 2, 6, 7}, 0}, /* Top */
    {{4, 5, 1, 0}, 2}  /* Bottom */
};

int main(int argc, char **argv) {
    GSurface *screen = gfx_screen();
    gfx_target(screen);

    float ax = 0.4f, ay = 0.6f;
    bool auto_spin = true;

    bool running = true;
    while (running) {
        if (plusos->key_poll()) {
            char ch = plusos->key_get();
            if (ch == 'q' || ch == 27) break;
            if (ch == 'a' || ch == 28) { ay -= 0.15f; auto_spin = false; }
            if (ch == 'd' || ch == 29) { ay += 0.15f; auto_spin = false; }
            if (ch == 'w' || ch == 30) { ax -= 0.15f; auto_spin = false; }
            if (ch == 's' || ch == 31) { ax += 0.15f; auto_spin = false; }
            if (ch == ' ') auto_spin = !auto_spin;
        }

        if (auto_spin) {
            ay += 0.04f;
            ax += 0.02f;
        }

        /* Render */
        gfx_fill(0, 0, 512, 342, 0);

        /* Rotate vertices */
        float cos_y = cosf(ay), sin_y = sinf(ay);
        float cos_x = cosf(ax), sin_x = sinf(ax);

        int sx[8], sy[8];
        float rot_z[8];

        for (int i = 0; i < 8; i++) {
            /* Yaw (Y rotation) */
            float x1 = cube_verts[i].x * cos_y + cube_verts[i].z * sin_y;
            float y1 = cube_verts[i].y;
            float z1 = -cube_verts[i].x * sin_y + cube_verts[i].z * cos_y;

            /* Pitch (X rotation) */
            float x2 = x1;
            float y2 = y1 * cos_x - z1 * sin_x;
            float z2 = y1 * sin_x + z1 * cos_x + 3.8f; /* Camera distance */

            rot_z[i] = z2;

            /* Project to 2D */
            float fov = 320.0f;
            sx[i] = 256 + (int)((x2 / z2) * fov);
            sy[i] = 171 - (int)((y2 / z2) * fov);
        }

        /* Draw faces with backface culling */
        for (int f = 0; f < 6; f++) {
            int v0 = cube_faces[f].v[0];
            int v1 = cube_faces[f].v[1];
            int v2 = cube_faces[f].v[2];
            int v3 = cube_faces[f].v[3];

            /* Cross product 2D for culling */
            int cross = (sx[v1] - sx[v0]) * (sy[v2] - sy[v0]) - (sy[v1] - sy[v0]) * (sx[v2] - sx[v0]);
            if (cross > 0) {
                /* Face is visible, draw wireframe & grid divisions (Rubik's cube facets) */
                gfx_line(sx[v0], sy[v0], sx[v1], sy[v1], 1);
                gfx_line(sx[v1], sy[v1], sx[v2], sy[v2], 1);
                gfx_line(sx[v2], sy[v2], sx[v3], sy[v3], 1);
                gfx_line(sx[v3], sy[v3], sx[v0], sy[v0], 1);

                /* 1/3 and 2/3 grid lines */
                for (int d = 1; d <= 2; d++) {
                    int px1 = sx[v0] + (sx[v1] - sx[v0]) * d / 3;
                    int py1 = sy[v0] + (sy[v1] - sy[v0]) * d / 3;
                    int px2 = sx[v3] + (sx[v2] - sx[v3]) * d / 3;
                    int py2 = sy[v3] + (sy[v2] - sy[v3]) * d / 3;
                    gfx_line(px1, py1, px2, py2, 1);

                    int qx1 = sx[v0] + (sx[v3] - sx[v0]) * d / 3;
                    int qy1 = sy[v0] + (sy[v3] - sy[v0]) * d / 3;
                    int qx2 = sx[v1] + (sx[v2] - sx[v1]) * d / 3;
                    int qy2 = sy[v1] + (sy[v2] - sy[v1]) * d / 3;
                    gfx_line(qx1, qy1, qx2, qy2, 1);
                }
            }
        }

        gfx_text(15, 15, "3D RUBIK'S CUBE DEMO - PLUSOS", 1);
        gfx_text(15, 30, "WASD / ARROWS: ROTATE  SPACE: SPIN  Q: QUIT", 1);

        sleep_ticks(2);
    }

    return 0;
}
