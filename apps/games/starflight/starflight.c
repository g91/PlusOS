/* starflight.c - 3D Rail Space Shooter for PlusOS */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_STARS 60
#define MAX_ENEMIES 4
#define MAX_LASERS 8

typedef struct {
    float x, y, z;
} Star;

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    bool active;
} Enemy;

typedef struct {
    float x, y, z;
    bool active;
} Laser;

static Star stars[MAX_STARS];
static Enemy enemies[MAX_ENEMIES];
static Laser lasers[MAX_LASERS];
static int reticle_x = 256, reticle_y = 171;
static int score = 0;
static int shield = 100;

static void reset_enemy(Enemy *e) {
    e->x = (float)((rand() % 40) - 20);
    e->y = (float)((rand() % 30) - 15);
    e->z = 50.0f + (float)(rand() % 30);
    e->vx = ((float)(rand() % 10) - 5.0f) * 0.1f;
    e->vy = ((float)(rand() % 10) - 5.0f) * 0.1f;
    e->vz = -0.8f - ((float)(rand() % 5) * 0.1f);
    e->active = true;
}

int main(int argc, char **argv) {
    gfx_target(gfx_backbuffer());

    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = (float)((rand() % 80) - 40);
        stars[i].y = (float)((rand() % 60) - 30);
        stars[i].z = 5.0f + (float)(rand() % 60);
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        reset_enemy(&enemies[i]);
    }

    bool running = true;
    while (running) {
        int kp_ = plusos->key_poll();
        if (kp_) {
            char ch = (char)kp_;
            if (ch == 'q' || ch == 27) break;
            if (ch == 'a' || ch == 28) reticle_x -= 10;
            if (ch == 'd' || ch == 29) reticle_x += 10;
            if (ch == 'w' || ch == 30) reticle_y -= 10;
            if (ch == 's' || ch == 31) reticle_y += 10;
            if (ch == ' ') {
                for (int i = 0; i < MAX_LASERS; i++) {
                    if (!lasers[i].active) {
                        lasers[i].x = (float)(reticle_x - 256) / 10.0f;
                        lasers[i].y = -(float)(reticle_y - 171) / 10.0f;
                        lasers[i].z = 2.0f;
                        lasers[i].active = true;
                        break;
                    }
                }
            }
        }

        if (reticle_x < 40) reticle_x = 40;
        if (reticle_x > 472) reticle_x = 472;
        if (reticle_y < 40) reticle_y = 40;
        if (reticle_y > 302) reticle_y = 302;

        /* Update stars */
        for (int i = 0; i < MAX_STARS; i++) {
            stars[i].z -= 1.2f;
            if (stars[i].z <= 1.0f) {
                stars[i].x = (float)((rand() % 80) - 40);
                stars[i].y = (float)((rand() % 60) - 30);
                stars[i].z = 60.0f;
            }
        }

        /* Update lasers */
        for (int i = 0; i < MAX_LASERS; i++) {
            if (lasers[i].active) {
                lasers[i].z += 2.5f;
                if (lasers[i].z > 60.0f) lasers[i].active = false;
            }
        }

        /* Update enemies */
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].active) continue;
            enemies[i].x += enemies[i].vx;
            enemies[i].y += enemies[i].vy;
            enemies[i].z += enemies[i].vz;

            /* Check hit by laser */
            for (int l = 0; l < MAX_LASERS; l++) {
                if (lasers[l].active) {
                    float dx = enemies[i].x - lasers[l].x;
                    float dy = enemies[i].y - lasers[l].y;
                    float dz = enemies[i].z - lasers[l].z;
                    if (dx*dx + dy*dy + dz*dz < 9.0f) {
                        score += 100;
                        lasers[l].active = false;
                        reset_enemy(&enemies[i]);
                        break;
                    }
                }
            }

            if (enemies[i].z <= 1.5f) {
                if (fabsf(enemies[i].x) < 4.0f && fabsf(enemies[i].y) < 3.0f) {
                    shield -= 20;
                }
                reset_enemy(&enemies[i]);
            }
        }

        /* Render */
        gfx_fill(0, 0, 512, 342, 0);

        /* Stars */
        for (int i = 0; i < MAX_STARS; i++) {
            if (stars[i].z > 0.5f) {
                int sx = 256 + (int)((stars[i].x / stars[i].z) * 260.0f);
                int sy = 171 - (int)((stars[i].y / stars[i].z) * 260.0f);
                if (sx >= 0 && sx < 512 && sy >= 0 && sy < 342) {
                    gfx_pixel(sx, sy, 1);
                }
            }
        }

        /* Lasers */
        for (int i = 0; i < MAX_LASERS; i++) {
            if (lasers[i].active && lasers[i].z > 0.5f) {
                int sx = 256 + (int)((lasers[i].x / lasers[i].z) * 260.0f);
                int sy = 171 - (int)((lasers[i].y / lasers[i].z) * 260.0f);
                if (sx >= 2 && sx < 510 && sy >= 2 && sy < 340) {
                    gfx_pixel(sx, sy, 1);
                    gfx_pixel(sx+1, sy, 1);
                    gfx_pixel(sx, sy+1, 1);
                    gfx_pixel(sx+1, sy+1, 1);
                }
            }
        }

        /* Enemies */
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].active || enemies[i].z <= 1.0f) continue;
            int ex = 256 + (int)((enemies[i].x / enemies[i].z) * 260.0f);
            int ey = 171 - (int)((enemies[i].y / enemies[i].z) * 260.0f);
            int s = (int)(260.0f / enemies[i].z);
            if (s < 2) s = 2;
            if (s > 40) s = 40;

            if (ex - s >= 0 && ex + s < 512 && ey - s >= 0 && ey + s < 342) {
                /* Diamond body */
                gfx_line(ex - s, ey, ex, ey - s/2, 1);
                gfx_line(ex, ey - s/2, ex + s, ey, 1);
                gfx_line(ex + s, ey, ex, ey + s/2, 1);
                gfx_line(ex, ey + s/2, ex - s, ey, 1);
                /* Wings */
                gfx_line(ex - s, ey - s, ex - s, ey + s, 1);
                gfx_line(ex + s, ey - s, ex + s, ey + s, 1);
            }
        }

        /* Crosshair */
        gfx_frame(reticle_x - 8, reticle_y - 8, reticle_x + 8, reticle_y + 8);
        gfx_line(reticle_x - 14, reticle_y, reticle_x - 4, reticle_y, 1);
        gfx_line(reticle_x + 4, reticle_y, reticle_x + 14, reticle_y, 1);
        gfx_line(reticle_x, reticle_y - 14, reticle_x, reticle_y - 4, 1);
        gfx_line(reticle_x, reticle_y + 4, reticle_x, reticle_y + 14, 1);

        /* HUD Cockpit struts */
        gfx_line(0, 341, 120, 290, 1);
        gfx_line(511, 341, 391, 290, 1);
        gfx_line(120, 290, 391, 290, 1);

        gfx_text(10, 10, "STARFLIGHT 3D - PLUSOS (WASD: AIM, SPACE: FIRE, Q: QUIT)", 0);
        char buf[32];
        snprintf(buf, sizeof(buf), "SCORE: %d  SHIELD: %d", score, shield);
        gfx_text(10, 24, buf, 0);

        gfx_present();
        sleep_ticks(1);
    }

    return 0;
}
