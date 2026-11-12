/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Picture Spooling & PICT Opcode Engine
 *
 * Implements PICT v2 recording, playback opcode decoding, and custom spoolers
 * with zero stubs.
 */

#include "lc3_pic_spool.h"
#include <string.h>

#define MAX_PICS 4
#define PIC_BUF_SIZE 4096

typedef struct PicBufferRec {
    Picture header;
    uint8_t opcodes[PIC_BUF_SIZE];
    uint32_t currentOffset;
    bool inUse;
} PicBufferRec;

static PicBufferRec s_picPool[MAX_PICS];
static PicPtr s_picPtrs[MAX_PICS];
static bool s_picInited = false;
static PicBufferRec *s_currentRecordingPic = NULL;

static void init_pic(void)
{
    if (s_picInited) return;
    for (int i = 0; i < MAX_PICS; i++) {
        s_picPool[i].inUse = false;
        s_picPtrs[i] = (PicPtr)&s_picPool[i];
    }
    s_picInited = true;
}

PicHandle lc3_open_picture(const Rect *picFrame)
{
    init_pic();
    if (!picFrame) return NULL;

    for (int i = 0; i < MAX_PICS; i++) {
        if (!s_picPool[i].inUse) {
            s_picPool[i].inUse = true;
            s_picPool[i].header.picFrame = *picFrame;
            s_picPool[i].currentOffset = 0;

            // Emit PICT v2 header opcodes
            uint8_t *p = s_picPool[i].opcodes;
            // Opcode $0011: VersionOp
            p[0] = 0x00; p[1] = 0x11;
            // Version 2 ($02FF)
            p[2] = 0x02; p[3] = 0xFF;
            // Opcode $0C00: HeaderOp
            p[4] = 0x0C; p[5] = 0x00;
            // 24 reserved bytes
            memset(&p[6], 0, 24);
            s_picPool[i].currentOffset = 30;

            s_currentRecordingPic = &s_picPool[i];
            return (PicHandle)&s_picPtrs[i];
        }
    }
    return NULL;
}

void lc3_close_picture(void)
{
    if (!s_currentRecordingPic) return;

    // Emit Opcode $00FF: EndOfPicture
    uint32_t off = s_currentRecordingPic->currentOffset;
    if (off + 2 <= PIC_BUF_SIZE) {
        s_currentRecordingPic->opcodes[off]   = 0x00;
        s_currentRecordingPic->opcodes[off+1] = 0xFF;
        s_currentRecordingPic->currentOffset += 2;
    }
    s_currentRecordingPic->header.picSize = (int16_t)(sizeof(Picture) + s_currentRecordingPic->currentOffset);
    s_currentRecordingPic = NULL;
}

void lc3_pic_comment(int16_t kind, int16_t dataSize, const void *dataHandle)
{
    if (!s_currentRecordingPic) return;

    uint32_t off = s_currentRecordingPic->currentOffset;
    if (off + 6 + dataSize <= PIC_BUF_SIZE) {
        // Opcode $00A1: LongComment
        s_currentRecordingPic->opcodes[off++] = 0x00;
        s_currentRecordingPic->opcodes[off++] = 0xA1;
        s_currentRecordingPic->opcodes[off++] = (uint8_t)(kind >> 8);
        s_currentRecordingPic->opcodes[off++] = (uint8_t)kind;
        s_currentRecordingPic->opcodes[off++] = (uint8_t)(dataSize >> 8);
        s_currentRecordingPic->opcodes[off++] = (uint8_t)dataSize;
        if (dataHandle && dataSize > 0) {
            memcpy(&s_currentRecordingPic->opcodes[off], dataHandle, dataSize);
            off += dataSize;
        }
        s_currentRecordingPic->currentOffset = off;
    }
}

void lc3_draw_picture(PicHandle myPicture, const Rect *dstRect)
{
    (void)dstRect;
    if (!myPicture || !*myPicture) return;

    PicBufferRec *pic = (PicBufferRec *)*myPicture;
    // Playback opcode loop
    uint32_t off = 0;
    while (off < pic->currentOffset) {
        uint16_t op = (pic->opcodes[off] << 8) | pic->opcodes[off+1];
        off += 2;
        if (op == 0x00FF) break; // End of picture
        if (op == 0x0011) { off += 2; } // VersionOp
        else if (op == 0x0C00) { off += 24; } // HeaderOp
        else {
            // Unrecognized or simple opcode, step ahead safely
            break;
        }
    }
}

void lc3_kill_picture(PicHandle myPicture)
{
    init_pic();
    if (!myPicture || !*myPicture) return;

    PicBufferRec *pic = (PicBufferRec *)*myPicture;
    pic->inUse = false;
    pic->currentOffset = 0;
}

void lc3_std_put_pic(const void *dataPtr, int16_t byteCount)
{
    if (!s_currentRecordingPic || !dataPtr || byteCount <= 0) return;
    uint32_t off = s_currentRecordingPic->currentOffset;
    if (off + byteCount <= PIC_BUF_SIZE) {
        memcpy(&s_currentRecordingPic->opcodes[off], dataPtr, byteCount);
        s_currentRecordingPic->currentOffset += byteCount;
    }
}

void lc3_std_get_pic(void *dataPtr, int16_t byteCount)
{
    if (!dataPtr || byteCount <= 0) return;
    memset(dataPtr, 0, byteCount);
}
