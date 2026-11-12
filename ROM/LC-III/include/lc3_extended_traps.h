/* lc3_extended_traps.h - Macintosh LC III Extended Dispatch Traps ($A1xx, $A2xx, $A9xx)
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x001390 - 0x15C9C8
 */
#ifndef LC3_EXTENDED_TRAPS_H
#define LC3_EXTENDED_TRAPS_H

#include <stdint.h>
#include <stdbool.h>

/* Trap Numbers */
#define TRAP_OS_A161         0xA161 /* Sonora DMA Arbiter */
#define TRAP_OS_A162         0xA162 /* Sonora Interrupt Mask */
#define TRAP_OS_A163         0xA163 /* 68030 Cache Burst Mode */
#define TRAP_OS_A164         0xA164 /* PDS Bus Timeout Config */
#define TRAP_OS_A165         0xA165 /* Fast Page Mode RAM Config */
#define TRAP_OS_A166         0xA166 /* Video VRAM Interleave Control */
#define TRAP_OS_A167         0xA167 /* SCSI Transfer Throttle */
#define TRAP_OS_A168         0xA168 /* Sonora Audio FIFO Config */
#define TRAP_OS_A169         0xA169 /* Egret Shift Rate Config */
#define TRAP_OS_A16A         0xA16A /* HW Power Rail Interrogate */
#define TRAP_OS_A16B         0xA16B /* PDS Card Interrupt Demux */
#define TRAP_OS_A16C         0xA16C /* Soft Clock Prescaler */
#define TRAP_OS_A16D         0xA16D /* Diagnostic Serial Pipe */
#define TRAP_OS_A16E         0xA16E /* NuBus/PDS Block Transfer */
#define TRAP_OS_A16F         0xA16F /* 68030 MMU Root Ptr Reload */
#define TRAP_OS_A170         0xA170 /* Video Refresh Strobe Hook */
#define TRAP_OS_A171         0xA171 /* Fast Memory Copy DMA */
#define TRAP_OS_A172         0xA172 /* Emergency Shutdown Handler */

#define TRAP_OS_A22E         0xA22E /* Egret Multi-Packet Queue */
#define TRAP_OS_A22F         0xA22F /* Egret Device Status Query */
#define TRAP_OS_A230         0xA230 /* Egret Set Keyboard Repeat */
#define TRAP_OS_A231         0xA231 /* Egret Power Manager Control */
#define TRAP_OS_A232         0xA232 /* Egret ADB Auto-Poll Hook */
#define TRAP_OS_A233         0xA233 /* Egret Direct Command Stream */
#define TRAP_OS_A234         0xA234 /* Egret Flush Transceiver */
#define TRAP_OS_A235         0xA235 /* Egret Extended PRAM Access */
#define TRAP_OS_A236         0xA236 /* PMMU Page Table Invalidate */
#define TRAP_OS_A237         0xA237 /* System 7 B-Tree Buffer Lock */
#define TRAP_OS_A238         0xA238 /* System 7 B-Tree Buffer Unlock */
#define TRAP_OS_A239         0xA239 /* File System Cache Flush */
#define TRAP_OS_A23A         0xA23A /* System Heap Expand Check */
#define TRAP_OS_A23B         0xA23B /* Virtual Memory Paging Sync */
#define TRAP_OS_A23C         0xA23C /* Deferred Interprocess Event */
#define TRAP_OS_A23D         0xA23D /* SCSI Reconnect Interrogate */
#define TRAP_OS_A23E         0xA23E /* SCSI Disconnect Handler */
#define TRAP_OS_A23F         0xA23F /* SCSI Arbitration Backoff */
#define TRAP_OS_A240         0xA240 /* Dynamic Stack Grow Limit */
#define TRAP_OS_A241         0xA241 /* Process Switch Save Context */
#define TRAP_OS_A242         0xA242 /* Process Switch Restore Context */
#define TRAP_OS_A243         0xA243 /* System 7 Process Sched Event */
#define TRAP_OS_A244         0xA244 /* High-Res Timer Microticks */
#define TRAP_OS_A245         0xA245 /* PDS Auxiliary Slot Config */

#define TRAP_TB_A9A6         0xA9A6 /* PopUp Menu Layout Builder */
#define TRAP_TB_A9A7         0xA9A7 /* PopUp Menu Hilite Tracker */
#define TRAP_TB_A9A8         0xA9A8 /* Color Icon Suite Load */
#define TRAP_TB_A9A9         0xA9A9 /* Color Icon Suite Plot */
#define TRAP_TB_A9AA         0xA9AA /* Help Manager Balloon Show */
#define TRAP_TB_A9AB         0xA9AB /* Help Manager Balloon Hide */
#define TRAP_TB_A9AC         0xA9AC /* System 7 Edition Mgr Register */
#define TRAP_TB_A9AD         0xA9AD /* System 7 Edition Mgr Unregister */
#define TRAP_TB_A9AE         0xA9AE /* TrueType Font Scaler Init */
#define TRAP_TB_A9AF         0xA9AF /* TrueType Scaler Render Glyph */
#define TRAP_TB_A9B0         0xA9B0 /* TrueType Scaler Measure Width */
#define TRAP_TB_A9B1         0xA9B1 /* TrueType Scaler Get Metrics */
#define TRAP_TB_A9B2         0xA9B2 /* Color Window Proxy Draw */
#define TRAP_TB_A9B3         0xA9B3 /* Custom WDEF Color Blitter */
#define TRAP_TB_A9B4         0xA9B4 /* Custom CDEF Color Blitter */
#define TRAP_TB_A9B5         0xA9B5 /* Color Menu Bar Render */
#define TRAP_TB_A9B6         0xA9B6 /* Color Menu Bar Item Hilite */
#define TRAP_TB_A9B7         0xA9B7 /* Standard File System 7 Dialog */
#define TRAP_TB_A9B8         0xA9B8 /* Standard File Get Custom Reply */
#define TRAP_TB_A9B9         0xA9B9 /* Standard File Nav File List */
#define TRAP_TB_A9BA         0xA9BA /* System 7 Folder Mgr Find Folder */
#define TRAP_TB_A9BB         0xA9BB /* System 7 Folder Mgr Release */
#define TRAP_TB_A9BC         0xA9BC /* Sound Manager 3.0 Mixer Open */
#define TRAP_TB_A9BD         0xA9BD /* Sound Manager 3.0 Rate Convert */
#define TRAP_TB_A9BE         0xA9BE /* Sound Manager 3.0 Channel Sync */
#define TRAP_TB_A9BF         0xA9BF /* Sound Manager 3.0 Mixer Close */
#define TRAP_TB_A9C0         0xA9C0 /* Color QuickDraw PixPat Fill */
#define TRAP_TB_A9C1         0xA9C1 /* Color QuickDraw MakeRGBPat */
#define TRAP_TB_A9C2         0xA9C2 /* Color QuickDraw CopyDeepMask */

/* Trap Dispatch Dispatcher */
int32_t lc3_dispatch_extended_os_trap(uint16_t trapWord, uint32_t d0, uint32_t a0, uint32_t a1);
int32_t lc3_dispatch_extended_tb_trap(uint16_t trapWord, void *stackFrame);

#endif /* LC3_EXTENDED_TRAPS_H */
