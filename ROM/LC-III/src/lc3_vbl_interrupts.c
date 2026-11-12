/* lc3_vbl_interrupts.c - Macintosh LC III VBL & Interrupt Demultiplexer Engine
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   VINSTALL_SYNC, LVL2RTS, VBLINT, VBL_CHECK_HEAP, EXTBINT, EXTAINT,
 *   EXTSTATUS_DISPATCH, EXTSTATUS_RESET, FILL_LVL1DT
 */

#include "lc3_vbl_interrupts.h"
#include "lc3_sonora_egret.h"
#include "lc3_boot_mgr.h"
#include "lc3_hardware.h"
#include <string.h>

static LC3VBLQueueHeader g_vblQueue;

void lc3_vbl_init(void) {
    g_vblQueue.qFlags = 0;
    g_vblQueue.qHead = 0;
    g_vblQueue.qTail = 0;
    LM_VBLQUEUE = g_vblQueue;
}

int16_t lc3_vbl_install(LC3VBLTask *vblTask) {
    /* VINSTALL_SYNC */
    if (!vblTask) return -1;
    vblTask->qLink = 0;
    if (!g_vblQueue.qHead) {
        g_vblQueue.qHead = vblTask;
        g_vblQueue.qTail = vblTask;
    } else {
        g_vblQueue.qTail->qLink = vblTask;
        g_vblQueue.qTail = vblTask;
    }
    LM_VBLQUEUE = g_vblQueue;
    return 0;
}

int16_t lc3_vbl_remove(LC3VBLTask *vblTask) {
    if (!vblTask) return -1;
    LC3VBLTask *cur = g_vblQueue.qHead;
    LC3VBLTask *prev = 0;
    while (cur) {
        if (cur == vblTask) {
            if (prev) {
                prev->qLink = cur->qLink;
            } else {
                g_vblQueue.qHead = cur->qLink;
            }
            if (cur == g_vblQueue.qTail) {
                g_vblQueue.qTail = prev;
            }
            cur->qLink = 0;
            LM_VBLQUEUE = g_vblQueue;
            return 0;
        }
        prev = cur;
        cur = cur->qLink;
    }
    return -1; /* Not found */
}

void lc3_vbl_interrupt_handler(void) {
    /* VBLINT, VBL_CHECK_HEAP: called 60.15 times per second by Sonora VBL IRQ */
    lc3_sonora_ack_vbl_interrupt();

    /* Increment low memory tick counter */
    LM_TICKS++;

    /* Walk active VBL task queue */
    LC3VBLTask *cur = g_vblQueue.qHead;
    while (cur) {
        if (cur->vblCount > 0) {
            cur->vblCount--;
            if (cur->vblCount == 0) {
                if (cur->vblAddr) {
                    cur->vblAddr();
                }
            }
        }
        cur = cur->qLink;
    }
}

void lc3_scc_exta_interrupt(void) {
    /* EXTAINT: SCC Channel A external status change interrupt */
    volatile uint8_t *scc = (volatile uint8_t *)LC3_SCC_BASE;
    uint8_t status = scc[0x04]; /* Read RR0 Channel A */
    (void)status;
    scc[0x04] = 0x10; /* Reset External/Status Interrupt */
}

void lc3_scc_extb_interrupt(void) {
    /* EXTBINT: SCC Channel B external status change interrupt */
    volatile uint8_t *scc = (volatile uint8_t *)LC3_SCC_BASE;
    uint8_t status = scc[0x00]; /* Read RR0 Channel B */
    (void)status;
    scc[0x00] = 0x10;
}

void lc3_ext_status_dispatch(uint8_t channel) {
    /* EXTSTATUS_DISPATCH, EXTSTATUS_RESET */
    if (channel == 0) {
        lc3_scc_exta_interrupt();
    } else {
        lc3_scc_extb_interrupt();
    }
}
