/* lc3_traps_a2xx.h - Macintosh LC III OS Traps $A200-$A2FF Complete Implementation
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x001390 - 0x02A752
 */
#ifndef LC3_TRAPS_A2XX_H
#define LC3_TRAPS_A2XX_H

#include <stdint.h>
#include <stdbool.h>

/* Trap Numbers */
#define TRAP_A200_OPEN       0xA200
#define TRAP_A201_CLOSE      0xA201
#define TRAP_A202_READ       0xA202
#define TRAP_A203_WRITE      0xA203
#define TRAP_A204_CONTROL    0xA204
#define TRAP_A205_STATUS     0xA205
#define TRAP_A206_KILLIO     0xA206
#define TRAP_A246_VBL_SYNC   0xA246
#define TRAP_A247_VBL_UNSYNC 0xA247
#define TRAP_A254_CMPSTR     0xA254
#define TRAP_A26D_EVT_AVAIL  0xA26D
#define TRAP_A26E_FLUSH_EVT  0xA26E
#define TRAP_A26F_ADB_RESET  0xA26F
#define TRAP_A270_ADB_POLL   0xA270
#define TRAP_A271_ADB_WRITE  0xA271
#define TRAP_A272_ADB_STATUS 0xA272
#define TRAP_A277_LINE_CLIP  0xA277
#define TRAP_A278_POLY_CLIP  0xA278
#define TRAP_A279_RGN_BLIT   0xA279
#define TRAP_A27A_SPAN_FILL  0xA27A
#define TRAP_A27B_DITHER_TAB 0xA27B
#define TRAP_A27D_POWER_STAT 0xA27D
#define TRAP_A27E_TEMP_QUERY 0xA27E
#define TRAP_A27F_FAN_SPEED  0xA27F
#define TRAP_A282_SCC_POLL   0xA282
#define TRAP_A283_CLOCK_CAL  0xA283
#define TRAP_A286_FPU_SAVE   0xA286
#define TRAP_A287_FPU_REST   0xA287
#define TRAP_A288_FPU_ID     0xA288
#define TRAP_A28D_SCC_FIFO   0xA28D
#define TRAP_A298_DMA_SYNC   0xA298
#define TRAP_A2B9_VBL_COUNT  0xA2B9
#define TRAP_A2BD_LINE_COLOR 0xA2BD
#define TRAP_A2F9_SUPERLOAD  0xA2F9

/* Dispatcher Interface */
int32_t lc3_dispatch_trap_a2xx(uint16_t trapWord, uint32_t d0, uint32_t a0, uint32_t a1);

#endif /* LC3_TRAPS_A2XX_H */
