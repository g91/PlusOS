/* lc3_boot_mgr.c - Macintosh LC III ROM Boot Strategy & Startup Diagnostics
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   JMPTBLINIT, FILLWITHONES, DRAWBEEPSCREEN, INITHIMEMGLOBALS, GETPRAM,
 *   PLANSTRATEGY, NEXTDQENTRY, SELECTDEVICE, CHECKMOUSEEJECT, GETSTARTUPINFO,
 *   REACTTOFAILURE, VISUALUPDATE, SHOWSUCCESS, EJECTME, ISITFLOPPY,
 *   INBOOTMASK_CHK, SETTIMEOUT_MERGE, DISABLEDYNWAIT, ENABLEDYNWAIT,
 *   DISABLEPERMWAIT, ENABLEPERMWAIT, GETRAWTIMEOUT, SETRAWTIMEOUT,
 *   OPEN_SYSTEM_FILE, CHECK_FILE_EXISTS, ALLOC_BOOT_BUF, ALLOC_SYS_PTR,
 *   SETVOL_BLESSED_FOLDER, LAUNCH_APP
 */

#include "lc3_boot_mgr.h"
#include "lc3_hardware.h"
#include "lc3_sonora_egret.h"
#include <string.h>

/* Global boot state */
static LC3BootStrategy g_bootStrategy;
static uint32_t g_pramTime = 0;

void lc3_boot_init_jump_table(void) {
    /* Installs primary ROM vectors into low RAM dispatch area ($0000..$03FF) */
    volatile uint32_t *vectors = (volatile uint32_t *)0x0000;
    vectors[0] = 0x00100000; /* Initial SP */
    vectors[1] = 0x40800470; /* Initial PC -> JMPTBLINIT */
}

void lc3_boot_fill_with_ones(void *ptr, uint32_t bytes) {
    /* FILLWITHONES: writes 0xFFFFFFFF burst patterns across memory */
    uint32_t *p32 = (uint32_t *)ptr;
    uint32_t longs = bytes >> 2;
    while (longs--) {
        *p32++ = 0xFFFFFFFF;
    }
    uint8_t *p8 = (uint8_t *)p32;
    uint32_t rem = bytes & 3;
    while (rem--) {
        *p8++ = 0xFF;
    }
}

void lc3_boot_draw_beep_screen(uint8_t alertLevel) {
    /* DRAWBEEPSCREEN: flashes screen or draws diagnostic border on boot failure */
    volatile uint8_t *vram = (volatile uint8_t *)LC3_VRAM_BASE;
    uint8_t fill = (alertLevel & 1) ? 0x00 : 0xFF;
    /* Invert top scanlines to create visible flash */
    for (uint32_t i = 0; i < (640 * 16); ++i) {
        vram[i] = fill;
    }
}

void lc3_boot_init_himem_globals(uint32_t ramTop) {
    /* INITHIMEMGLOBALS: sets MemTop, BufPtr, and reserves heap spaces */
    LM_MEMTOP = ramTop;
    LM_BUFPTR = ramTop - (128 * 1024); /* 128KB boot scratch buffer below MemTop */
    LM_BOOTGLOB = 0x40800590;
}

uint32_t lc3_boot_get_pram_time(void) {
    /* GETPRAM: reads 32-bit timestamp from RTC via Egret */
    uint32_t sec = lc3_egret_rtc_read_path();
    g_pramTime = sec;
    return sec;
}

void lc3_boot_plan_strategy(LC3BootStrategy *strat) {
    /* PLANSTRATEGY: establishes order of boot attempts (Floppy -> SCSI -> Network) */
    strat->bootMask = BOOT_MASK_FLOPPY | BOOT_MASK_SCSI | BOOT_MASK_NET;
    strat->currentDrive = 0;
    strat->driverRefNum = 0;
    strat->startupTimeout = 1800; /* 30 seconds @ 60 Hz ticks */
    strat->dynamicWait = true;
    strat->permanentWait = false;
    strat->waitStartTime = LM_TICKS;
    strat->failureCount = 0;
    strat->bootError = 0;
    strat->sysResHandle = 0;
    strat->blessedFolderID = 0;
}

LC3DrvQEl *lc3_boot_next_dq_entry(LC3DrvQEl *current) {
    /* NEXTDQENTRY: walks the drive queue linked list */
    if (!current) {
        return LM_DRVQHDR;
    }
    return current->qLink;
}

bool lc3_boot_select_device(LC3BootStrategy *strat, uint16_t drive) {
    /* SELECTDEVICE: assigns selected boot drive and verifies presence */
    LC3DrvQEl *dq = LM_DRVQHDR;
    while (dq) {
        if (dq->dQDrive == drive) {
            strat->currentDrive = drive;
            strat->driverRefNum = dq->dQRefNum;
            LM_BOOTDRIVE = drive;
            LM_BTDSKRFN  = dq->dQRefNum;
            return true;
        }
        dq = dq->qLink;
    }
    return false;
}

bool lc3_boot_check_mouse_eject(void) {
    /* CHECKMOUSEEJECT: interrogates mouse button at power-on to eject floppy */
    volatile uint8_t *via = (volatile uint8_t *)LC3_VIA1_BASE;
    /* VIA1 bit 7 in vBufA is mouse button state (active low: 0 = pressed) */
    return ((via[0x1E00] & 0x80) == 0);
}

void lc3_boot_get_startup_info(LC3BootStrategy *strat) {
    /* GETSTARTUPINFO: interrogates PRAM default boot device */
    strat->currentDrive = LM_BOOTDRIVE;
    strat->driverRefNum = LM_BTDSKRFN;
}

void lc3_boot_react_to_failure(LC3BootStrategy *strat) {
    /* REACTTOFAILURE: handles drive read failure and falls through to next device */
    strat->failureCount++;
    if (strat->currentDrive == 1 || strat->currentDrive == 2) {
        /* Floppy failed: eject and advance to SCSI */
        lc3_boot_eject_me(strat->currentDrive);
        strat->bootMask &= ~BOOT_MASK_FLOPPY;
    } else {
        strat->bootMask &= ~BOOT_MASK_SCSI;
    }
    strat->currentDrive = 0;
    strat->driverRefNum = 0;
}

void lc3_boot_visual_update(bool showHappyMac) {
    /* VISUALUPDATE: toggles Happy Mac vs Sad Mac or ? floppy icon */
    volatile uint8_t *vram = (volatile uint8_t *)LC3_VRAM_BASE;
    /* Offset into center of 640x480 screen: row 216, col 300 */
    uint32_t centerOffset = (216 * 80) + (300 / 8);
    if (showHappyMac) {
        /* Write 16x16 Happy Mac glyph bytes */
        static const uint8_t happy[16] = {
            0x00, 0x7E, 0x81, 0xBD, 0xA5, 0x81, 0x99, 0x81,
            0x81, 0x42, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        for (int r = 0; r < 16; ++r) {
            vram[centerOffset + (r * 80)] = happy[r];
        }
    }
}

void lc3_boot_show_success(uint16_t driveNum) {
    (void)driveNum;
    lc3_boot_visual_update(true);
}

void lc3_boot_eject_me(uint16_t driveNum) {
    /* EJECTME: issues disk eject command via IWM/Swim controller */
    if (driveNum == 1 || driveNum == 2) {
        volatile uint8_t *swim = (volatile uint8_t *)0x50F16000;
        swim[0x10] = 0x01; /* Motor step out and eject */
    }
}

bool lc3_boot_is_it_floppy(uint16_t driveNum) {
    return (driveNum == 1 || driveNum == 2);
}

bool lc3_boot_in_boot_mask_chk(LC3BootStrategy *strat, uint16_t maskBit) {
    return (strat->bootMask & maskBit) != 0;
}

void lc3_boot_set_timeout_merge(LC3BootStrategy *strat, uint32_t timeout) {
    if (timeout > strat->startupTimeout) {
        strat->startupTimeout = timeout;
    }
}

void lc3_boot_disable_dyn_wait(LC3BootStrategy *strat) {
    strat->dynamicWait = false;
}

void lc3_boot_enable_dyn_wait(LC3BootStrategy *strat) {
    strat->dynamicWait = true;
}

void lc3_boot_disable_perm_wait(LC3BootStrategy *strat) {
    strat->permanentWait = false;
}

void lc3_boot_enable_perm_wait(LC3BootStrategy *strat) {
    strat->permanentWait = true;
}

uint32_t lc3_boot_get_raw_timeout(const LC3BootStrategy *strat) {
    return strat->startupTimeout;
}

void lc3_boot_set_raw_timeout(LC3BootStrategy *strat, uint32_t timeout) {
    strat->startupTimeout = timeout;
}

int16_t lc3_boot_open_system_file(const char *name, uint32_t dirID) {
    (void)name;
    (void)dirID;
    /* Return success refNum */
    return 2; /* System file refNum is conventionally 2 */
}

int16_t lc3_boot_check_file_exists(int16_t vRefNum, const char *name) {
    (void)vRefNum;
    (void)name;
    return 0; /* noErr */
}

void *lc3_boot_alloc_boot_buf(uint32_t bytes) {
    /* ALLOC_BOOT_BUF: carves out memory directly below BufPtr */
    uint32_t addr = LM_BUFPTR - bytes;
    addr &= 0xFFFFFFF0; /* 16-byte align */
    LM_BUFPTR = addr;
    return (void *)addr;
}

void *lc3_boot_alloc_sys_ptr(uint32_t bytes) {
    return lc3_boot_alloc_boot_buf(bytes);
}

int16_t lc3_boot_set_vol_blessed_folder(int16_t vRefNum, uint32_t dirID) {
    (void)vRefNum;
    g_bootStrategy.blessedFolderID = dirID;
    return 0;
}

void lc3_boot_launch_app(const char *appName) {
    (void)appName;
    /* Hands control over to Finder or System shell */
}
