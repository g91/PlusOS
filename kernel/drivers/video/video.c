/* video.c - the PlusOS video driver.
 *
 * The compact Macs have no graphics accelerator: video is a 512x342, 1-bit
 * region of RAM (ScrnBase, 64 bytes a row) scanned out at ~60 Hz by the board's
 * logic. Drawing straight into it while the beam is reading it is what makes the
 * screen flicker - you see the erase before the repaint.
 *
 * This driver gives programs an off-screen "back buffer" to draw into, then
 * copies a finished frame to the live screen in one fast pass ("present", a
 * software page flip). The copy runs top-to-bottom, the same direction as the
 * scan, and is started right after a vertical-blank tick, so the beam never
 * overtakes it and the screen changes in one piece - like a film frame.
 */
#include "kernel.h"

#define FB_ROWBYTES 64
#define FB_ROWS     342
#define FB_BYTES    (FB_ROWBYTES * FB_ROWS)      /* 21888, a multiple of 32 */

static uint8_t *front;                  /* the live screen (ScrnBase) */
static uint8_t *back;                   /* off-screen drawing buffer */

uint8_t *video_front(void) { return front; }
uint8_t *video_back(void)  { return back ? back : front; }

extern void video_copy(uint8_t *dst, const uint8_t *src, uint32_t n);   /* vblit.S */

/* whole 64-byte rows; lock CrsrBusy ($08CD) during the blit so VBL JCrsrTask
 * (InterruptHandlers.a) cannot XOR the hardware cursor mid-copy. */
static void blit(uint8_t *dst, const uint8_t *src, uint32_t n)
{
    uint8_t was_busy = LM_BYTE(0x8CD);
    LM_BYTE(0x8CD) = 1;
    video_copy(dst, src, n);
    LM_BYTE(0x8CD) = was_busy;
}

/* Wait for the start of the next vertical blank (the Ticks low-memory counter
 * is bumped by the 60 Hz VBL), so a present begins in the blanking interval. */
static void wait_vblank(void)
{
    volatile uint32_t *ticks = (volatile uint32_t *)0x16A;
    uint32_t t = *ticks;
    uint32_t spins = 0;
    while (*ticks == t && ++spins < 200000) { }      /* bounded, in case VBL is off */
}

void video_present(void)
{
    if (!back) return;
    wait_vblank();
    blit(front, back, FB_BYTES);
}

/* Present only rows [top, bottom) - cheaper for small updates (clock, an icon). */
void video_present_rect(int top, int bottom)
{
    if (!back) return;
    if (top < 0) top = 0;
    if (bottom > FB_ROWS) bottom = FB_ROWS;
    if (top >= bottom) return;
    blit(front + top * FB_ROWBYTES, back + top * FB_ROWBYTES,
         (uint32_t)(bottom - top) * FB_ROWBYTES);
}

void video_init(void)
{
    front = (uint8_t *)LM_LONG(0x824);              /* ScrnBase */
    back = kmalloc(FB_BYTES);
    if (back) memcpy(back, front, FB_BYTES);        /* start matching the screen */
}
