/* hwdetect.h - Universal Hardware Detection & HAL.
 *
 * Hardware decoder flags, CPU/MMU/FPU identification, and model descriptors.
 */
#ifndef PLUSOS_HWDETECT_H
#define PLUSOS_HWDETECT_H

#include <stdint.h>

/* Apple BoxFlag ($0CB3) hardware model constants */
#define BOX_UNKNOWN          0xFD
#define BOX_PLUS             0xFE
#define BOX_SE               0xFF
#define BOX_MACII            0x00
#define BOX_MACIIX           0x01
#define BOX_MACIICX          0x02
#define BOX_SE30             0x03
#define BOX_PORTABLE         0x04
#define BOX_MACIICI          0x05
#define BOX_MACIIFX          0x07
#define BOX_CLASSIC          0x0B
#define BOX_MACIISI          0x0C
#define BOX_MACLC            0x0D
#define BOX_QUADRA900        0x0E
#define BOX_POWERBOOK170     0x0F
#define BOX_QUADRA700        0x10
#define BOX_CLASSICII        0x11
#define BOX_POWERBOOK100     0x12
#define BOX_POWERBOOK140     0x13
#define BOX_QUADRA950        0x14
#define BOX_LCIII            0x15
#define BOX_PBDUO210         0x17
#define BOX_CENTRIS650       0x18
#define BOX_COLORCLASSIC     0x19
#define BOX_PBDUO230         0x1A
#define BOX_POWERBOOK180     0x1B
#define BOX_POWERBOOK160     0x1C
#define BOX_QUADRA800        0x1D
#define BOX_QUADRA650        0x1E
#define BOX_MACLCII          0x1F
#define BOX_VAIL16           0x22
#define BOX_QUADRA610        0x2F
#define BOX_QUADRA840AV      0x48
#define BOX_QUADRA605        0x58

/* Gestalt machine type IDs */
#define GMACH_CLASSIC_SE_FD  1
#define GMACH_MACPLUS        4
#define GMACH_MACSE          5
#define GMACH_MACII          6
#define GMACH_MACIIX         7
#define GMACH_MACIICX        8
#define GMACH_MACSE030       9
#define GMACH_PORTABLE       10
#define GMACH_MACIICI        11
#define GMACH_MACIIFX        13
#define GMACH_MACCLASSIC     17
#define GMACH_MACIISI        18
#define GMACH_MACLC          19
#define GMACH_QUADRA900      20
#define GMACH_POWERBOOK170   21
#define GMACH_QUADRA700      22
#define GMACH_CLASSICII      23
#define GMACH_POWERBOOK100   24
#define GMACH_POWERBOOK140   25
#define GMACH_QUADRA950      26
#define GMACH_MACLCIII       27
#define GMACH_PBDUO210       29
#define GMACH_CENTRIS650     30
#define GMACH_PBDUO230       32
#define GMACH_POWERBOOK180   33
#define GMACH_POWERBOOK160   34
#define GMACH_QUADRA800      35
#define GMACH_QUADRA650      36
#define GMACH_MACLCII        37
#define GMACH_COLORCLASSIC   49
#define GMACH_QUADRA610      53
#define GMACH_CENTRIS660AV   60
#define GMACH_QUADRA840AV    78

/* Decoder / bus architecture classes */
#define HW_MMU_NONE          0
#define HW_MMU_AMU           1
#define HW_MMU_68851         2
#define HW_MMU_68030         3
#define HW_MMU_68040         4

#define HW_FPU_NONE          0
#define HW_FPU_68881         1
#define HW_FPU_68882         2
#define HW_FPU_68040         3

typedef struct PlusHWInfo {
    uint8_t   cpu_flag;          /* CPUFlag ($012F): 0=000..4=040 */
    uint8_t   fpu_type;          /* HW_FPU_* */
    uint8_t   mmu_type;          /* HW_MMU_* */
    uint8_t   box_flag;          /* BoxFlag ($0CB3) */

    uint16_t  rom85;             /* ROM85 ($028E) */
    uint16_t  hw_cfg_flags;      /* HWCfgFlags ($0B22) */
    uint32_t  rom_base;          /* ROMBase ($02AE) */
    uint32_t  rom_size;          /* detected ROM size in bytes */
    uint16_t  rom_version;       /* ROM version word from ROMBase+8 */
    uint32_t  vbr;               /* Vector Base Register (0 on 68000) */
    uint32_t  univ_info_ptr;     /* UnivInfoPtr ($0DD8) if universal ROM */

    int32_t   gestalt_mach;      /* Gestalt 'mach' ID */
    const char *machine_name;    /* human-readable Apple model name */

    /* Capability flags discovered via Universal/Gestalt/ROM trap checks */
    uint8_t   has_universal_rom;
    uint8_t   has_via1;
    uint8_t   has_via2;
    uint8_t   has_rbv;
    uint8_t   has_adb;
    uint8_t   has_scc;
    uint8_t   has_scsi;
    uint8_t   has_scsi96;
    uint8_t   has_asc;
    uint8_t   has_classic_sound;
    uint8_t   has_slot_mgr;
    uint8_t   has_color_qd;
    uint8_t   has_xpram;
    uint8_t   has_soft_poweroff;
    uint8_t   has_gestalt_trap;
    uint8_t   has_time_mgr_ext;
    uint8_t   is_32bit_mode;
    uint8_t   is_32bit_capable;
    uint8_t   nubus_slots;       /* logical NuBus slot count */
    uint32_t  nubus_slot_mask;   /* bitmap of present slots */

    /* Hardware register base addresses (from ROM low-mem / Gestalt / probe) */
    uint32_t  via1_base;         /* $01D4 */
    uint32_t  via2_base;         /* $0CEC (or RBV) */
    uint32_t  rbv_base;          /* RBV if present */
    uint32_t  scc_rd_base;       /* $01D8 */
    uint32_t  scc_wr_base;       /* $01DC */
    uint32_t  iwm_base;          /* $01E0 */
    uint32_t  scsi_base;         /* $0C00 (on non-Plus) / $580000 on Plus */
    uint32_t  asc_base;          /* $0CC0 */
} PlusHWInfo;

extern PlusHWInfo g_hw;

void        hwdetect_init(void);
const char *hw_machine_name(void);
const char *hw_cpu_name(void);
const char *hw_fpu_name(void);
const char *hw_mmu_name(void);
int         hw_rom_has_trap(uint16_t trap);
int         hw_call_rom_gestalt(uint32_t selector, int32_t *out);

/* hwprobe.S */
uint8_t     hw_probe_cpu_level(void);
uint32_t    hw_get_vbr(void);
int         hw_probe_read8(uint32_t addr, uint8_t *out);
int         hw_probe_read32(uint32_t addr, uint32_t *out);
int         hw_probe_fpu_raw(void);

#endif /* PLUSOS_HWDETECT_H */
