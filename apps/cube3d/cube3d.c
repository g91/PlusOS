/* cube3d.c - 3D Cubes, 3D Hexagons, Polyhedra & Polygon Showcase for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plus3d/plus3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

enum {
    MODE_CUBE = 0,
    MODE_HEX_PRISM,
    MODE_PYRAMID,
    MODE_OCTAHEDRON,
    MODE_TORUS,
    MODE_POLY_GALLERY,
    MODE_COUNT
};

static const char * const mode_names[MODE_COUNT] = {
    "1: 3D CUBE / SQUARE",
    "2: 3D HEXAGON PRISM",
    "3: 3D PYRAMID",
    "4: 3D OCTAHEDRON",
    "5: 3D TORUS RING",
    "6: POLYGONS & HEXAGONS"
};

static void draw_poly_gallery(int deg, int solid_mode)
{
    /* Row 1: Regular N-gons (Triangle, Square, Pentagon, Hexagon, Octagon) */
    static const int sides[5] = { 3, 4, 5, 6, 8 };
    static const char * const labels[5] = { "TRIANGLE", "SQUARE", "PENTAGON", "HEXAGON", "OCTAGON" };
    for (int i = 0; i < 5; i++) {
        int cx = 56 + i * 100;
        int cy = 108;
        int shade = solid_mode ? (3 + i * 3) : 0;
        if (solid_mode)
            gfx_ngon_fill(cx, cy, 38, sides[i], deg + i * 15, shade);
        gfx_ngon_frame(cx, cy, 38, sides[i], deg + i * 15, 1);
        gfx_ngon_frame(cx, cy, 20, sides[i], -deg, 1);
        int tw = gfx_text_width(labels[i]);
        gfx_text(cx - tw / 2, cy + 46, labels[i], 0);
    }

    /* Row 2 left: Honeycomb Hexagonal Grid */
    int hcx = 135, hcy = 245, hr = 24;
    static const int hex_dx[7] = { 0,  37,  37,   0, -37, -37,   0 };
    static const int hex_dy[7] = { 0, -21,  21,  42,  21, -21, -42 };
    for (int i = 0; i < 7; i++) {
        int shade = solid_mode ? ((i * 2 + (deg / 15)) % 14 + 2) : -1;
        gfx_hexagon(hcx + hex_dx[i], hcy + hex_dy[i], hr, 0, shade, 1);
    }
    gfx_text(75, 305, "HONEYCOMB HEXAGONS", 0);

    /* Row 2 center: Rotating Nested 3D-Effect Squares & Hexagons */
    int scx = 285, scy = 245;
    for (int k = 4; k >= 1; k--) {
        int r = k * 12;
        int a = deg * (k & 1 ? 1 : -1) + k * 15;
        if (solid_mode)
            gfx_ngon_fill(scx, scy, r, (k & 1) ? 6 : 4, a, k * 3);
        gfx_ngon_frame(scx, scy, r, (k & 1) ? 6 : 4, a, 1);
    }
    gfx_text(233, 305, "NESTED HEX/SQUARES", 0);

    /* Row 2 right: 10-vertex Concave Star Polygon */
    int stx = 425, sty = 245;
    int star_pts[20];
    for (int i = 0; i < 10; i++) {
        int r = (i & 1) ? 20 : 46;
        int one_pt[6];
        gfx_ngon_pts(stx, sty, r, 3, deg + i * 36 - 90, one_pt);
        star_pts[i * 2]     = one_pt[0];
        star_pts[i * 2 + 1] = one_pt[1];
    }
    if (solid_mode)
        gfx_poly_dither(star_pts, 10, 9);
    gfx_poly_frame(star_pts, 10, 1);
    gfx_text(377, 305, "CONCAVE STAR POLY", 0);
}

/* Screen rows the animated 3D view may touch (between the two chrome bars). */
enum { AREA_T = 21, AREA_B = 313 };

/* FPS readout, top-right of the title bar - redrawn every 15 frames */
static void draw_stats(const char *fps_buf)
{
    gfx_fill(424, 2, 512, 18, 1);
    gfx_text(432, 6, fps_buf, 1);
}

/* Static chrome: drawn once, and again only when the shape/mode changes */
static void draw_chrome(int mode, const char *fps_buf)
{
    gfx_fill(0, 0, 512, 342, 0);
    gfx_fill(0, 0, 512, 20, 1);
    gfx_text(8, 6, "PLUS3D & POLYGON ENGINE", 1);
    gfx_text(224, 6, mode_names[mode], 1);
    gfx_hline(0, 512, 324, 1);
    gfx_text(8, 329, "1-6/TAB: SHAPE  F: SHADE/WIRE  WASD: ROTATE  SPC: SPIN  Q: QUIT", 0);
    draw_stats(fps_buf);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    gfx_target(gfx_backbuffer());

    renderer_init();
    Camera3D cam;
    cam.pos      = vec3(0.0f, 0.0f, -3.6f);
    cam.target   = vec3(0.0f, 0.0f, 0.0f);
    cam.up       = vec3(0.0f, 1.0f, 0.0f);
    cam.fov      = 1.0f;
    cam.view_mat = mat4_lookat(cam.pos, cam.target, cam.up);
    cam.proj_mat = mat4_perspective(1.05f, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.5f, 50.0f);
    renderer_set_camera(&cam);

    Mesh3D *meshes[5];
    meshes[0] = mesh_build_cube(1.65f);
    meshes[1] = mesh_build_hex_prism(1.35f, 1.45f);
    meshes[2] = mesh_build_pyramid(1.9f, 1.9f);
    meshes[3] = mesh_build_octahedron(1.55f);
    meshes[4] = mesh_build_torus(1.05f, 0.42f, 9, 6);

    int mode = MODE_CUBE;
    int render_flags = RENDER_SOLID_FLAT | RENDER_WIREFRAME;
    float ax = 0.45f, ay = 0.65f, az = 0.0f;
    int deg = 0;
    bool auto_spin = true;

    /* Live FPS meter (integer; ticks() is the 60.15 Hz VBL counter). */
    uint32_t fps_frames = 0;
    uint32_t fps_t0 = plusos->ticks();
    int fps_x10 = 0;        /* frames-per-second * 10 */
    char fps_buf[24];
    fps_buf[0] = 0;

    /* Dirty rectangles: the static chrome (title, bottom bar) is drawn once;
     * each frame erases only what the 3D view drew last frame, and only the
     * rows that changed are copied to the screen. */
    int need_chrome = 1;
    int pl = 0, pt = AREA_T, pr = 512, pb = AREA_B;     /* last frame's 3D bbox */
    uint32_t frame_ctr = 0;

    for (;;) {
        int old_mode = mode, old_flags = render_flags;
        frame_ctr++;
        /* key_poll() CONSUMES and returns the key (0 = none). Calling
         * key_get() after it would block waiting for a second keypress. */
        for (int ch; (ch = plusos->key_poll()) != 0; ) {
            if (ch == 'q' || ch == 'Q' || ch == 27) goto done;
            if (ch == 'a' || ch == 28) { ay -= 0.14f; auto_spin = false; }
            if (ch == 'd' || ch == 29) { ay += 0.14f; auto_spin = false; }
            if (ch == 'w' || ch == 30) { ax -= 0.14f; auto_spin = false; }
            if (ch == 's' || ch == 31) { ax += 0.14f; auto_spin = false; }
            if (ch == ' ') auto_spin = !auto_spin;
            if (ch == '\t' || ch == 'm' || ch == 'M') mode = (mode + 1) % MODE_COUNT;
            if (ch >= '1' && ch <= '6') mode = ch - '1';
            if (ch == 'f' || ch == 'F') {
                if (render_flags == (RENDER_SOLID_FLAT | RENDER_WIREFRAME))
                    render_flags = RENDER_WIREFRAME;
                else if (render_flags == RENDER_WIREFRAME)
                    render_flags = RENDER_SOLID_FLAT;
                else
                    render_flags = RENDER_SOLID_FLAT | RENDER_WIREFRAME;
            }
        }

        if (mode != old_mode || render_flags != old_flags) need_chrome = 1;

        if (auto_spin) {
            ay += 0.045f;
            ax += 0.025f;
            az += 0.015f;
            deg = (deg + 3) % 360;
        }

        int dtop = 9999, dbot = -1;                 /* dirty row band this frame */
#define DIRTY(t, b) do { if ((t) < dtop) dtop = (t); if ((b) > dbot) dbot = (b); } while (0)
        int solid = (render_flags & RENDER_SOLID_FLAT) != 0;
        int stats_changed = 0;

        if (need_chrome) {
            draw_chrome(mode, fps_buf);
            pl = 0; pt = AREA_T; pr = 512; pb = AREA_B;
        }

        if (mode == MODE_POLY_GALLERY) {
            gfx_fill(0, AREA_T, 512, AREA_B, 0);
            draw_poly_gallery(deg, solid);
            DIRTY(AREA_T, AREA_B);
        } else {
            /* erase only last frame's 3D bounding box */
            gfx_fill(pl, pt, pr, pb, 0);
            DIRTY(pt, pb);

            /* Decorative spinning 2D shapes: cosmetic, so refresh only every 4th
             * frame (at full rate they were ~20% of the whole frame). */
            if ((frame_ctr & 3) == 0) {
                gfx_fill(16, 36, 78, 106, 0);
                gfx_fill(436, 36, 498, 106, 0);
                gfx_hexagon(46, 66, 26, deg, solid ? 4 : -1, 1);
                gfx_ngon_fill(466, 66, 26, 4, -deg, solid ? 6 : 0);
                gfx_ngon_frame(466, 66, 26, 4, -deg, 1);
                gfx_text(22, 98, "2D HEX", 0);
                gfx_text(442, 98, "2D SQR", 0);
                DIRTY(36, 106);
            }

            Mat4 model = mat4_mul(mat4_mul(mat4_rotate_x(ax), mat4_rotate_y(ay)), mat4_rotate_z(az));
            gfx_clip(0, AREA_T, 512, AREA_B);       /* the 3D view never touches the chrome */
            renderer_draw_mesh(meshes[mode], model, render_flags);
            gfx_noclip();

            /* this frame's 3D bounding box, from the projected vertices */
            Mesh3D *mm = meshes[mode];
            int nl = 9999, nt = 9999, nr = -9999, nb = -9999;
            for (int i = 0; i < mm->num_vertices; i++) {
                int x = FIX_INT(mm->projected[i].x), y = FIX_INT(mm->projected[i].y);
                if (x < nl) nl = x;
                if (x > nr) nr = x;
                if (y < nt) nt = y;
                if (y > nb) nb = y;
            }
            nl -= 2; nt -= 2; nr += 3; nb += 3;
            if (nl < 0) nl = 0;
            if (nr > 512) nr = 512;
            if (nt < AREA_T) nt = AREA_T;
            if (nb > AREA_B) nb = AREA_B;
            if (nl >= nr || nt >= nb) { nl = nr = 0; nt = nb = AREA_T; }
            DIRTY(nt, nb);
            pl = nl; pt = nt; pr = nr; pb = nb;
        }

        /* FPS meter: recompute every 15 frames from the VBL tick delta */
        fps_frames++;
        if (fps_frames >= 15) {
            uint32_t now = plusos->ticks();
            uint32_t dt = now - fps_t0;
            if (dt == 0) dt = 1;
            /* fps*10 = frames * 601 / dt  (60.15 Hz * 10 ~= 601) */
            fps_x10 = (int)((fps_frames * 601UL) / dt);
            snprintf(fps_buf, sizeof(fps_buf), "%d.%d FPS", fps_x10 / 10, fps_x10 % 10);
            fps_frames = 0;
            fps_t0 = now;
            draw_stats(fps_buf);
            stats_changed = 1;
        }

        /* copy only the changed rows to the screen (no vblank wait, no sleep) */
        if (need_chrome) {
            gfx_present();
            need_chrome = 0;
        } else {
            if (dbot > dtop) gfx_present_rows(dtop, dbot);
            if (stats_changed) gfx_present_rows(0, 20);   /* FPS in the title bar */
        }
#undef DIRTY
    }

done:
    for (int i = 0; i < 5; i++) mesh_free(meshes[i]);
    return 0;
}
