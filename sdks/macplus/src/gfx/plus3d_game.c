/* plus3d_game.c - Collision Detection and 3D Game Entity Engine */
#include "plus3d_internal.h"

Entity3D game_entities[MAX_GAME_ENTITIES];
int num_game_entities = 0;


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
