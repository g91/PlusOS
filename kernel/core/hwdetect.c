/* hwdetect.c - Universal Hardware Detection & HAL.
 *
 * Implements standard Macintosh runtime hardware detection:
 *   1. Inspects CPUFlag ($012F), ROMBase ($02AE), ROM85 ($028E),
 *      HWCfgFlags ($0B22), MMU32Bit ($0CB2), BoxFlag ($0CB3), and
 *      UnivInfoPtr ($0DD8).
 *   2. Checks ROM trap table availability (ToolTable $0C00 on Plus vs $0E00 on
 *      later Macs, OSTable $0400) against _Unimplemented ($A89F).
 *   3. Queries ROM _Gestalt ($A1AD) when present, and safely bus-error-probes
 *      fallback chip addresses (VIA1/VIA2/RBV/ASC/FPU) when running on older
 *      ROMs (Plus, SE, Classic, Mac II).
 */
#include "kernel.h"
#include "hwdetect.h"

PlusHWInfo g_hw;

static int valid_ptr(uint32_t p)
{
    return p >= 0x100 && p != 0xFFFFFFFFUL && p != 0x00000000UL && (p & 1u) == 0;
}

static void **trap_slot_for(uint16_t trap)
{
    if (trap & 0x0800u) {
        /* 64K / Mac Plus (ROM85 < 0 || rom[8]==0 on <020): ToolTable at $0C00; later Macs $0E00 */
        const uint8_t *rom = (const uint8_t *)LM_LONG(0x2AE);
        uint32_t tool_base = 0x0E00UL;
        if ((int16_t)LM_WORD(0x28E) < 0 || (LM_BYTE(0x12F) < 2 && (!rom || rom[8] == 0)))
            tool_base = 0x0C00UL;
        return (void **)tool_base + (trap & 0x3FFu);
    }
    return (void **)0x0400UL + (trap & 0xFFu);
}

int hw_rom_has_trap(uint16_t trap)
{
    void *unimpl = *(void **)trap_slot_for(0xA89Fu); /* _Unimplemented */
    /* Some very old Plus ROMs store odd or 0 for unimplemented slots; normal traps != unimpl */
    void *slot = *trap_slot_for(trap);
    if (!slot || slot == unimpl) return 0;
    /* Toolbox trap index >= 0x200 only exists when expanded ToolTable is installed */
    if ((trap & 0x0800u) && (trap & 0x03FFu) >= 0x200u) {
        void **tb = trap_slot_for(0xA800u);
        if ((uint32_t)(uintptr_t)tb == 0x0C00UL) return 0;
    }
    return 1;
}

int hw_call_rom_gestalt(uint32_t selector, int32_t *out)
{
    if (!g_hw.has_gestalt_trap) return -5551;
    register uint32_t d0 __asm__("d0") = selector;
    register int32_t  a0 __asm__("a0") = 0;
    __asm__ volatile(".short 0xA1AD"
                     : "+d"(d0), "=a"(a0)
                     :
                     : "d1", "d2", "a1", "cc", "memory");
    if ((int16_t)d0 == 0 && out) *out = a0;
    return (int16_t)d0;
}

typedef struct { int32_t mach; const char *name; uint8_t is_compact; uint8_t nubus; } BoxMeta;

static void resolve_model(void)
{
    const uint8_t *rom = (const uint8_t *)g_hw.rom_base;
    uint8_t idbyte = rom ? rom[8] : 0;
    uint8_t box    = g_hw.box_flag;

    /* 1. Ask ROM _Gestalt('mach') first if Universal ROM implements it */
    int32_t gmach = 0;
    if (g_hw.has_gestalt_trap && hw_call_rom_gestalt(0x6D616368UL /* 'mach' */, &gmach) == 0 && gmach > 0)
        g_hw.gestalt_mach = gmach;

    /* 2. Check Mac Plus ROM signature (ROMBase+$08 == 0x00 and 128K ROM / CPUFlag < 2) */
    if (idbyte == 0 && g_hw.cpu_flag < 2 && box != BOX_SE && box != BOX_CLASSIC && box != BOX_PORTABLE) {
        g_hw.box_flag = BOX_PLUS;
        g_hw.gestalt_mach = GMACH_MACPLUS;
        g_hw.machine_name = "Macintosh Plus";
        return;
    }

    switch (box) {
    case BOX_PLUS:         g_hw.gestalt_mach = GMACH_MACPLUS;      g_hw.machine_name = "Macintosh Plus"; break;
    case BOX_SE:           g_hw.gestalt_mach = GMACH_MACSE;        g_hw.machine_name = "Macintosh SE"; break;
    case BOX_MACII:        g_hw.gestalt_mach = GMACH_MACII;        g_hw.machine_name = "Macintosh II"; g_hw.nubus_slots = 6; break;
    case BOX_MACIIX:       g_hw.gestalt_mach = GMACH_MACIIX;       g_hw.machine_name = "Macintosh IIx"; g_hw.nubus_slots = 6; break;
    case BOX_MACIICX:      g_hw.gestalt_mach = GMACH_MACIICX;      g_hw.machine_name = "Macintosh IIcx"; g_hw.nubus_slots = 3; break;
    case BOX_SE30:         g_hw.gestalt_mach = GMACH_MACSE030;     g_hw.machine_name = "Macintosh SE/30"; break;
    case BOX_PORTABLE:     g_hw.gestalt_mach = GMACH_PORTABLE;     g_hw.machine_name = "Macintosh Portable"; break;
    case BOX_MACIICI:      g_hw.gestalt_mach = GMACH_MACIICI;      g_hw.machine_name = "Macintosh IIci"; g_hw.nubus_slots = 3; break;
    case BOX_MACIIFX:      g_hw.gestalt_mach = GMACH_MACIIFX;      g_hw.machine_name = "Macintosh IIfx"; g_hw.nubus_slots = 6; break;
    case BOX_CLASSIC:      g_hw.gestalt_mach = GMACH_MACCLASSIC;   g_hw.machine_name = "Macintosh Classic"; break;
    case BOX_MACIISI:      g_hw.gestalt_mach = GMACH_MACIISI;      g_hw.machine_name = "Macintosh IIsi"; break;
    case BOX_MACLC:        g_hw.gestalt_mach = GMACH_MACLC;        g_hw.machine_name = "Macintosh LC"; break;
    case BOX_QUADRA900:    g_hw.gestalt_mach = GMACH_QUADRA900;    g_hw.machine_name = "Macintosh Quadra 900"; g_hw.nubus_slots = 5; break;
    case BOX_POWERBOOK170: g_hw.gestalt_mach = GMACH_POWERBOOK170; g_hw.machine_name = "Macintosh PowerBook 170"; break;
    case BOX_QUADRA700:    g_hw.gestalt_mach = GMACH_QUADRA700;    g_hw.machine_name = "Macintosh Quadra 700"; g_hw.nubus_slots = 2; break;
    case BOX_CLASSICII:    g_hw.gestalt_mach = GMACH_CLASSICII;    g_hw.machine_name = "Macintosh Classic II"; break;
    case BOX_POWERBOOK100: g_hw.gestalt_mach = GMACH_POWERBOOK100; g_hw.machine_name = "Macintosh PowerBook 100"; break;
    case BOX_POWERBOOK140: g_hw.gestalt_mach = GMACH_POWERBOOK140; g_hw.machine_name = "Macintosh PowerBook 140"; break;
    case BOX_QUADRA950:    g_hw.gestalt_mach = GMACH_QUADRA950;    g_hw.machine_name = "Macintosh Quadra 950"; g_hw.nubus_slots = 5; break;
    case BOX_LCIII:
    case BOX_VAIL16:       g_hw.gestalt_mach = GMACH_MACLCIII;     g_hw.machine_name = "Macintosh LC III"; break;
    case BOX_PBDUO210:     g_hw.gestalt_mach = GMACH_PBDUO210;     g_hw.machine_name = "PowerBook Duo 210"; break;
    case BOX_CENTRIS650:   g_hw.gestalt_mach = GMACH_CENTRIS650;   g_hw.machine_name = "Macintosh Centris 650"; g_hw.nubus_slots = 3; break;
    case BOX_COLORCLASSIC: g_hw.gestalt_mach = GMACH_COLORCLASSIC; g_hw.machine_name = "Macintosh Color Classic"; break;
    case BOX_PBDUO230:     g_hw.gestalt_mach = GMACH_PBDUO230;     g_hw.machine_name = "PowerBook Duo 230"; break;
    case BOX_POWERBOOK180: g_hw.gestalt_mach = GMACH_POWERBOOK180; g_hw.machine_name = "PowerBook 180"; break;
    case BOX_POWERBOOK160: g_hw.gestalt_mach = GMACH_POWERBOOK160; g_hw.machine_name = "PowerBook 160"; break;
    case BOX_QUADRA800:    g_hw.gestalt_mach = GMACH_QUADRA800;    g_hw.machine_name = "Macintosh Quadra 800"; g_hw.nubus_slots = 3; break;
    case BOX_QUADRA650:    g_hw.gestalt_mach = GMACH_QUADRA650;    g_hw.machine_name = "Macintosh Quadra 650"; g_hw.nubus_slots = 3; break;
    case BOX_MACLCII:      g_hw.gestalt_mach = GMACH_MACLCII;      g_hw.machine_name = "Macintosh LC II"; break;
    case BOX_QUADRA610:    g_hw.gestalt_mach = GMACH_QUADRA610;    g_hw.machine_name = "Macintosh Quadra 610"; break;
    case BOX_QUADRA840AV:  g_hw.gestalt_mach = GMACH_QUADRA840AV;  g_hw.machine_name = "Macintosh Quadra 840AV"; g_hw.nubus_slots = 3; break;
    case BOX_QUADRA605:    g_hw.gestalt_mach = GMACH_MACLC;        g_hw.machine_name = "Macintosh Quadra 605 / LC 475"; break;
    default:
        /* Pre-BoxFlag ROMs (SE / Classic / early II): fall back to ROMBase+9 */
        if (rom && rom[9] == 0x76) {
            g_hw.box_flag = BOX_SE;
            g_hw.gestalt_mach = GMACH_MACSE;
            g_hw.machine_name = "Macintosh SE";
        } else if (rom && rom[9] == 0x7C) {
            g_hw.box_flag = BOX_CLASSIC;
            g_hw.gestalt_mach = GMACH_MACCLASSIC;
            g_hw.machine_name = "Macintosh Classic";
        } else if (rom && rom[8] == 0x78) {
            g_hw.box_flag = BOX_MACII;
            g_hw.gestalt_mach = GMACH_MACII;
            g_hw.machine_name = "Macintosh II";
            g_hw.nubus_slots = 6;
        } else if (g_hw.cpu_flag >= 4) {
            g_hw.machine_name = "Macintosh Quadra / 68040";
        } else if (g_hw.cpu_flag >= 2) {
            g_hw.machine_name = "Macintosh (68020/030 Modular)";
        } else {
            g_hw.machine_name = "Macintosh (68k)";
        }
        break;
    }
}

void hwdetect_init(void)
{
    memset(&g_hw, 0, sizeof g_hw);

    g_hw.cpu_flag     = hw_probe_cpu_level();
    if (g_hw.cpu_flag > LM_BYTE(0x12F))
        LM_BYTE(0x12F) = g_hw.cpu_flag;
    g_hw.vbr          = hw_get_vbr();
    g_hw.rom_base     = LM_LONG(0x2AE);
    g_hw.rom85        = LM_WORD(0x28E);
    g_hw.hw_cfg_flags = LM_WORD(0xB22);
    g_hw.box_flag     = LM_BYTE(0xCB3);

    const uint8_t *rom = (const uint8_t *)g_hw.rom_base;
    if (rom) {
        g_hw.rom_version = *(const uint16_t *)(rom + 8);
        /* ROM size: ROMBase+$40 (Universal) or infer from ROM85/era */
        if ((int16_t)g_hw.rom85 < 0) {
            g_hw.rom_size = 65536UL;
        } else if (rom[8] == 0x00) {
            g_hw.rom_size = 131072UL;    /* Mac Plus 128K */
        } else if (rom[8] == 0x78) {
            g_hw.rom_size = 262144UL;    /* Mac SE / II 256K */
        } else {
            uint32_t rsz = *(const uint32_t *)(rom + 0x40);
            if (rsz == 0x80000UL || rsz == 0x100000UL || rsz == 0x200000UL || rsz == 0x400000UL)
                g_hw.rom_size = rsz;
            else
                g_hw.rom_size = (g_hw.rom_version >= 0x067C) ? 524288UL : 262144UL;
        }
    }

    g_hw.has_gestalt_trap = hw_rom_has_trap(0xA1AD);
    g_hw.has_slot_mgr     = hw_rom_has_trap(0xA06E);
    g_hw.has_adb          = (g_hw.hw_cfg_flags & (1u << 11)) != 0 || hw_rom_has_trap(0xA077);
    g_hw.has_time_mgr_ext = hw_rom_has_trap(0xA458) || g_hw.rom_version >= 0x067C;

    uint32_t uptr = LM_LONG(0xDD8);
    if (valid_ptr(uptr) && g_hw.rom_version >= 0x067C) {
        g_hw.univ_info_ptr = uptr;
        g_hw.has_universal_rom = 1;
    }

    resolve_model();
    g_hw.has_color_qd = (g_hw.cpu_flag >= 2 && g_hw.box_flag != BOX_PLUS &&
                         g_hw.box_flag != BOX_SE && g_hw.box_flag != BOX_CLASSIC) &&
                        hw_rom_has_trap(0xAA00);

    /* Low-memory base addresses (with safe architectural defaults) */
    g_hw.via1_base   = valid_ptr(LM_LONG(0x1D4)) ? LM_LONG(0x1D4) : 0xEFE1FEUL;
    g_hw.scc_rd_base = valid_ptr(LM_LONG(0x1D8)) ? LM_LONG(0x1D8) : (g_hw.box_flag == BOX_PLUS || g_hw.box_flag == BOX_SE ? 0x9FFFF8UL : 0x50004000UL);
    g_hw.scc_wr_base = valid_ptr(LM_LONG(0x1DC)) ? LM_LONG(0x1DC) : (g_hw.box_flag == BOX_PLUS || g_hw.box_flag == BOX_SE ? 0xBFFFF9UL : 0x50004002UL);
    g_hw.iwm_base    = valid_ptr(LM_LONG(0x1E0)) ? LM_LONG(0x1E0) : 0xDFE1FFUL;

    if (g_hw.box_flag != BOX_PLUS && valid_ptr(LM_LONG(0xCEC)))
        g_hw.via2_base = LM_LONG(0xCEC);
    if (g_hw.box_flag != BOX_PLUS && valid_ptr(LM_LONG(0xCC0)))
        g_hw.asc_base = LM_LONG(0xCC0);
    if (g_hw.box_flag != BOX_PLUS && valid_ptr(LM_LONG(0xC00)) && g_hw.rom_version >= 0x0178)
        g_hw.scsi_base = LM_LONG(0xC00);
    else if (g_hw.box_flag == BOX_PLUS || g_hw.box_flag == BOX_SE)
        g_hw.scsi_base = 0x580000UL;

    /* Query ROM _Gestalt for authoritative hardware attributes when available */
    int32_t resp = 0;
    if (g_hw.has_gestalt_trap && hw_call_rom_gestalt(0x68647772UL /* 'hdwr' */, &resp) == 0) {
        g_hw.has_via1          = (resp & (1L << 0)) != 0;
        g_hw.has_via2          = (resp & (1L << 1)) != 0;
        g_hw.has_asc           = (resp & (1L << 3)) != 0;
        g_hw.has_scc           = (resp & (1L << 4)) != 0;
        g_hw.has_scsi          = (resp & (1L << 7)) != 0;
        g_hw.has_soft_poweroff = (resp & (1L << 19)) != 0;
        g_hw.has_scsi96        = ((resp & (1L << 21)) || (resp & (1L << 22)));
        if (resp & (1L << 24)) g_hw.has_universal_rom = 1;
        if (hw_call_rom_gestalt(0x76696131UL /* 'via1' */, &resp) == 0 && resp) g_hw.via1_base = (uint32_t)resp;
        if (hw_call_rom_gestalt(0x76696132UL /* 'via2' */, &resp) == 0 && resp) g_hw.via2_base = (uint32_t)resp;
        if (hw_call_rom_gestalt(0x72627620UL /* 'rbv ' */, &resp) == 0 && resp) { g_hw.rbv_base = (uint32_t)resp; g_hw.has_rbv = 1; }
        if (hw_call_rom_gestalt(0x73636372UL /* 'sccr' */, &resp) == 0 && resp) g_hw.scc_rd_base = (uint32_t)resp;
        if (hw_call_rom_gestalt(0x73636377UL /* 'sccw' */, &resp) == 0 && resp) g_hw.scc_wr_base = (uint32_t)resp;
        if (hw_call_rom_gestalt(0x6E756273UL /* 'nubs' */, &resp) == 0) g_hw.nubus_slots = (uint8_t)resp;
        if (hw_call_rom_gestalt(0x736C7463UL /* 'sltc' */, &resp) == 0) g_hw.nubus_slot_mask = (uint32_t)resp;
    } else {
        g_hw.has_via1 = 1;
        g_hw.has_scc  = 1;
        g_hw.has_scsi = 1;
        g_hw.has_via2 = (g_hw.box_flag != BOX_PLUS && g_hw.box_flag != BOX_SE && g_hw.box_flag != BOX_CLASSIC && g_hw.via2_base != 0);
        g_hw.has_asc  = (g_hw.box_flag != BOX_PLUS && g_hw.box_flag != BOX_SE && g_hw.asc_base != 0);
        g_hw.has_soft_poweroff = (g_hw.has_via2 && g_hw.box_flag != BOX_SE30 && g_hw.box_flag != BOX_MACLCII);
    }

    /* Compact Mac PWM SoundBuffer check (Plus / SE / Classic with SoundBase) */
    g_hw.has_classic_sound = (!g_hw.has_asc &&
                              (g_hw.box_flag == BOX_PLUS || g_hw.box_flag == BOX_SE ||
                               g_hw.box_flag == BOX_CLASSIC || g_hw.cpu_flag < 2) &&
                              valid_ptr(LM_LONG(0x266)));

    /* XPRAM (256-byte clock chip + startup device) exists on SE and all later Macs */
    g_hw.has_xpram = (g_hw.box_flag != BOX_PLUS) &&
                     (g_hw.box_flag == BOX_SE || g_hw.box_flag == BOX_CLASSIC || hw_rom_has_trap(0xA051));
    if (g_hw.box_flag != BOX_PLUS && g_hw.box_flag != BOX_SE && g_hw.box_flag != BOX_CLASSIC)
        g_hw.has_adb = 1;

    /* MMU & 32-bit mode (only 68020+ CPUs have 32-bit / PMMU capability) */
    g_hw.is_32bit_mode    = (g_hw.cpu_flag >= 2) && ((LM_BYTE(0xCB2) & 1u) != 0);
    g_hw.is_32bit_capable = (g_hw.cpu_flag >= 2 && g_hw.box_flag != BOX_MACII);
    if (g_hw.cpu_flag >= 4)      g_hw.mmu_type = HW_MMU_68040;
    else if (g_hw.cpu_flag == 3) g_hw.mmu_type = HW_MMU_68030;
    else if (g_hw.cpu_flag == 2) g_hw.mmu_type = (g_hw.hw_cfg_flags & 0x20u) ? HW_MMU_68851 : HW_MMU_NONE;

    /* FPU: check F-line safely via hw_probe_fpu_raw() */
    if (hw_probe_fpu_raw() == 0) {
        if (g_hw.cpu_flag >= 4)      g_hw.fpu_type = HW_FPU_68040;
        else if (g_hw.cpu_flag >= 3) g_hw.fpu_type = HW_FPU_68882;
        else                         g_hw.fpu_type = HW_FPU_68881;
    } else {
        g_hw.fpu_type = HW_FPU_NONE;
    }
}

const char *hw_machine_name(void)
{
    return g_hw.machine_name ? g_hw.machine_name : plusrom_machine_name();
}

const char *hw_cpu_name(void)
{
    switch (g_hw.cpu_flag) {
    case 0:  return "68000";
    case 1:  return "68010";
    case 2:  return "68020";
    case 3:  return "68030";
    case 4:  return "68040";
    default: return "680x0";
    }
}

const char *hw_fpu_name(void)
{
    switch (g_hw.fpu_type) {
    case HW_FPU_68881: return "68881";
    case HW_FPU_68882: return "68882";
    case HW_FPU_68040: return "68040 integrated";
    default:           return "none";
    }
}

const char *hw_mmu_name(void)
{
    switch (g_hw.mmu_type) {
    case HW_MMU_68040: return "68040 MMU";
    case HW_MMU_68030: return "68030 PMMU";
    case HW_MMU_68851: return "68851 PMMU";
    case HW_MMU_AMU:   return "AMU";
    default:           return "none";
    }
}
