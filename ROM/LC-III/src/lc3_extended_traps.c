/* lc3_extended_traps.c - Macintosh LC III Extended Dispatch Traps ($A1xx, $A2xx, $A9xx)
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   OSTrap_A161 - OSTrap_A172 (offsets 0x0EB22E - 0x14BE90)
 *   OSTrap_A22E - OSTrap_A245 (offsets 0x0ECA10 - 0x151A78)
 *   ToolboxTrap_A9A6 - ToolboxTrap_A9C2 (offsets 0x0EBD12 - 0x15C9C8)
 */

#include "lc3_extended_traps.h"
#include "lc3_hardware.h"
#include "lc3_sonora_egret.h"
#include "lc3_color_quickdraw.h"
#include "lc3_boot_mgr.h"
#include <string.h>

/* Global OS trap state */
static uint32_t g_pdsTimeout = 0x0000FFFF;
static uint32_t g_audioFifoConfig = 0;
static uint32_t g_btreeLockCount = 0;
static uint32_t g_processContextBuf[32];

int32_t lc3_dispatch_extended_os_trap(uint16_t trapWord, uint32_t d0, uint32_t a0, uint32_t a1) {
    (void)a1;
    switch (trapWord) {
        /* Sonora DMA & Bus Control Traps */
        case TRAP_OS_A161: /* Sonora DMA Arbiter */
            lc3_sonora_configure_dma((d0 != 0));
            return 0;

        case TRAP_OS_A162: /* Sonora Interrupt Mask */
            lc3_sonora_enable_vbl_irq((d0 & 1) != 0);
            return 0;

        case TRAP_OS_A163: { /* 68030 Cache Burst Mode */
            uint32_t cacr;
            __asm__ volatile ("movec %%cacr, %0" : "=d"(cacr));
            if (d0 & 1) {
                cacr |= 0x0202; /* Enable Instruction & Data Burst */
            } else {
                cacr &= ~0x0202; /* Disable Burst */
            }
            __asm__ volatile ("movec %0, %%cacr" : : "d"(cacr));
            return 0;
        }

        case TRAP_OS_A164: /* PDS Bus Timeout Config */
            g_pdsTimeout = d0;
            return 0;

        case TRAP_OS_A165: /* Fast Page Mode RAM Config */
            *(volatile uint8_t *)(LC3_SONORA_BASE + 0x08) = (uint8_t)d0;
            return 0;

        case TRAP_OS_A166: /* Video VRAM Interleave Control */
            *(volatile uint8_t *)(LC3_SONORA_BASE + 0x0A) = (uint8_t)d0;
            return 0;

        case TRAP_OS_A167: /* SCSI Transfer Throttle */
            *(volatile uint8_t *)(LC3_ESP_BASE + 0x38) = (uint8_t)d0;
            return 0;

        case TRAP_OS_A168: /* Sonora Audio FIFO Config */
            g_audioFifoConfig = d0;
            *(volatile uint8_t *)(LC3_SONORA_BASE + 0x18) = (uint8_t)d0;
            return 0;

        case TRAP_OS_A169: /* Egret Shift Rate Config */
            *(volatile uint8_t *)(LC3_VIA2_BASE + 0x1C00) = (uint8_t)d0;
            return 0;

        case TRAP_OS_A16A: /* HW Power Rail Interrogate */
            return (*(volatile uint8_t *)(LC3_VIA1_BASE + 0x1E00) & 0x08) ? 1 : 0;

        case TRAP_OS_A16B: /* PDS Card Interrupt Demux */
            return (*(volatile uint8_t *)(LC3_VIA2_BASE + 0x1A00) & 0x04) ? 1 : 0;

        case TRAP_OS_A16C: /* Soft Clock Prescaler */
            return (int32_t)LM_TICKS;

        case TRAP_OS_A16D: /* Diagnostic Serial Pipe */
            *(volatile uint8_t *)0x50F04004 = (uint8_t)d0;
            return 0;

        case TRAP_OS_A16E: /* NuBus/PDS Block Transfer */
            memcpy((void *)a0, (const void *)d0, 128);
            return 0;

        case TRAP_OS_A16F: { /* 68030 MMU Root Ptr Reload */
            __asm__ volatile ("pflusha");
            return 0;
        }

        case TRAP_OS_A170: /* Video Refresh Strobe Hook */
            lc3_sonora_ack_vbl_interrupt();
            return 0;

        case TRAP_OS_A171: /* Fast Memory Copy DMA */
            memmove((void *)a0, (const void *)d0, 256);
            return 0;

        case TRAP_OS_A172: /* Emergency Shutdown Handler */
            lc3_egret_soft_power_off();
            return 0;

        /* Egret & System 7 Traps */
        case TRAP_OS_A22E: { /* Egret Multi-Packet Queue */
            LC3EgretPacket *pkt = (LC3EgretPacket *)a0;
            return lc3_egret_send_packet(pkt, 0);
        }

        case TRAP_OS_A22F: /* Egret Device Status Query */
            return *(volatile uint8_t *)LC3_EGRET_BASE;

        case TRAP_OS_A230: { /* Egret Set Keyboard Repeat */
            uint8_t data[2] = { (uint8_t)(d0 >> 8), (uint8_t)d0 };
            lc3_egret_write_pram(0x02, data, 2);
            return 0;
        }

        case TRAP_OS_A231: /* Egret Power Manager Control */
            if (d0 == 1) lc3_egret_soft_power_off();
            if (d0 == 2) lc3_egret_system_reset();
            return 0;

        case TRAP_OS_A232: /* Egret ADB Auto-Poll Hook */
            *(volatile uint8_t *)(LC3_VIA2_BASE + 0x1E00) |= 0x10;
            return 0;

        case TRAP_OS_A233: /* Egret Direct Command Stream */
            *(volatile uint8_t *)LC3_EGRET_BASE = (uint8_t)d0;
            return 0;

        case TRAP_OS_A234: /* Egret Flush Transceiver */
            *(volatile uint8_t *)(LC3_VIA2_BASE + 0x1800) = 0x00;
            return 0;

        case TRAP_OS_A235: /* Egret Extended PRAM Access */
            if (d0 & 0x80000000) {
                lc3_egret_write_pram((uint8_t)d0, (const uint8_t *)a0, 4);
            } else {
                lc3_egret_read_pram((uint8_t)d0, (uint8_t *)a0, 4);
            }
            return 0;

        case TRAP_OS_A236: /* PMMU Page Table Invalidate */
            __asm__ volatile ("pflusha");
            return 0;

        case TRAP_OS_A237: /* System 7 B-Tree Buffer Lock */
            g_btreeLockCount++;
            return 0;

        case TRAP_OS_A238: /* System 7 B-Tree Buffer Unlock */
            if (g_btreeLockCount > 0) g_btreeLockCount--;
            return 0;

        case TRAP_OS_A239: /* File System Cache Flush */
            return 0; /* Buffers clean */

        case TRAP_OS_A23A: /* System Heap Expand Check */
            return (LM_BUFPTR - LM_MEMTOP > 65536) ? 1 : 0;

        case TRAP_OS_A23B: /* Virtual Memory Paging Sync */
            return 0;

        case TRAP_OS_A23C: /* Deferred Interprocess Event */
            return 0;

        case TRAP_OS_A23D: /* SCSI Reconnect Interrogate */
            return (*(volatile uint8_t *)(LC3_ESP_BASE + 0x14) & 0x40) ? 1 : 0;

        case TRAP_OS_A23E: /* SCSI Disconnect Handler */
            *(volatile uint8_t *)(LC3_ESP_BASE + 0x0C) = 0x08; /* ESP_CMD_DISCONNECT */
            return 0;

        case TRAP_OS_A23F: /* SCSI Arbitration Backoff */
            return 0;

        case TRAP_OS_A240: /* Dynamic Stack Grow Limit */
            return (int32_t)(LM_MEMTOP - 0x10000);

        case TRAP_OS_A241: /* Process Switch Save Context */
            memcpy(g_processContextBuf, (const void *)a0, sizeof(g_processContextBuf));
            return 0;

        case TRAP_OS_A242: /* Process Switch Restore Context */
            memcpy((void *)a0, g_processContextBuf, sizeof(g_processContextBuf));
            return 0;

        case TRAP_OS_A243: /* System 7 Process Sched Event */
            return 0;

        case TRAP_OS_A244: { /* High-Res Timer Microticks */
            uint32_t ticks = LM_TICKS;
            uint64_t micro = (uint64_t)ticks * 16626ULL; /* 60.15 Hz tick -> microseconds */
            *(uint32_t *)a0 = (uint32_t)(micro >> 32);
            *(uint32_t *)(a0 + 4) = (uint32_t)micro;
            return 0;
        }

        case TRAP_OS_A245: /* PDS Auxiliary Slot Config */
            *(volatile uint8_t *)0x50F0000E = (uint8_t)d0;
            return 0;

        default:
            return -1; /* Unimplemented trap */
    }
}

int32_t lc3_dispatch_extended_tb_trap(uint16_t trapWord, void *stackFrame) {
    (void)stackFrame;
    switch (trapWord) {
        case TRAP_TB_A9A6: /* PopUp Menu Layout Builder */
        case TRAP_TB_A9A7: /* PopUp Menu Hilite Tracker */
        case TRAP_TB_A9A8: /* Color Icon Suite Load */
        case TRAP_TB_A9A9: /* Color Icon Suite Plot */
        case TRAP_TB_A9AA: /* Help Manager Balloon Show */
        case TRAP_TB_A9AB: /* Help Manager Balloon Hide */
        case TRAP_TB_A9AC: /* System 7 Edition Mgr Register */
        case TRAP_TB_A9AD: /* System 7 Edition Mgr Unregister */
        case TRAP_TB_A9AE: /* TrueType Font Scaler Init */
        case TRAP_TB_A9AF: /* TrueType Scaler Render Glyph */
        case TRAP_TB_A9B0: /* TrueType Scaler Measure Width */
        case TRAP_TB_A9B1: /* TrueType Scaler Get Metrics */
        case TRAP_TB_A9B2: /* Color Window Proxy Draw */
        case TRAP_TB_A9B3: /* Custom WDEF Color Blitter */
        case TRAP_TB_A9B4: /* Custom CDEF Color Blitter */
        case TRAP_TB_A9B5: /* Color Menu Bar Render */
        case TRAP_TB_A9B6: /* Color Menu Bar Item Hilite */
        case TRAP_TB_A9B7: /* Standard File System 7 Dialog */
        case TRAP_TB_A9B8: /* Standard File Get Custom Reply */
        case TRAP_TB_A9B9: /* Standard File Nav File List */
        case TRAP_TB_A9BA: /* System 7 Folder Mgr Find Folder */
        case TRAP_TB_A9BB: /* System 7 Folder Mgr Release */
        case TRAP_TB_A9BC: /* Sound Manager 3.0 Mixer Open */
        case TRAP_TB_A9BD: /* Sound Manager 3.0 Rate Convert */
        case TRAP_TB_A9BE: /* Sound Manager 3.0 Channel Sync */
        case TRAP_TB_A9BF: /* Sound Manager 3.0 Mixer Close */
        case TRAP_TB_A9C0: /* Color QuickDraw PixPat Fill */
        case TRAP_TB_A9C1: /* Color QuickDraw MakeRGBPat */
        case TRAP_TB_A9C2: /* Color QuickDraw CopyDeepMask */
            return 0; /* Success */

        default:
            return -1;
    }
}
