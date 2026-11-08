#ifndef PLUS3D_INTERNAL_H
#define PLUS3D_INTERNAL_H

#include <plus3d/plus3d.h>
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

fix_t fix_sqrt(fix_t a);
void  ensure_trig(void);
fix_t fsin(float rad);
fix_t fcos(float rad);
Mat4  mat4_translate_fix(fix_t x, fix_t y, fix_t z);
Mat4  mat4_scale_fix(fix_t x, fix_t y, fix_t z);

#endif /* PLUS3D_INTERNAL_H */
