/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Picture Spooling & PICT Opcode Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_PIC_SPOOL_H
#define LC3_PIC_SPOOL_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} Rect;

typedef struct Picture {
    int16_t picSize;
    Rect picFrame;
    // PICT v2 opcodes follow: $0011 (VersionOp $02FF), $0C00 (HeaderOp), etc.
} Picture, *PicPtr, **PicHandle;

typedef void (*QDBytePutProcPtr)(uint8_t byteVal);
typedef uint8_t (*QDByteGetProcPtr)(void);

/* Picture API */
PicHandle lc3_open_picture(const Rect *picFrame);
void lc3_close_picture(void);
void lc3_pic_comment(int16_t kind, int16_t dataSize, const void *dataHandle);
void lc3_draw_picture(PicHandle myPicture, const Rect *dstRect);
void lc3_kill_picture(PicHandle myPicture);
void lc3_std_put_pic(const void *dataPtr, int16_t byteCount);
void lc3_std_get_pic(void *dataPtr, int16_t byteCount);

#endif /* LC3_PIC_SPOOL_H */
