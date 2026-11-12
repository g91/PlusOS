/* lc3_boot_mgr.h - Macintosh LC III ROM Boot Strategy & Startup Diagnostics
 * Fully reverse-engineered from LC III Universal ROM $067C (lc3_rom.map)
 * Offsets 0x000470 - 0x0021BC
 */
#ifndef LC3_BOOT_MGR_H
#define LC3_BOOT_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define BOOT_MASK_FLOPPY    0x01
#define BOOT_MASK_SCSI      0x02
#define BOOT_MASK_ROM       0x04
#define BOOT_MASK_NET       0x08

/* Boot Strategy State Record */
typedef struct {
    uint16_t bootMask;          /* active device mask */
    uint16_t currentDrive;       /* selected drive number */
    int16_t  driverRefNum;       /* selected driver refNum */
    uint32_t startupTimeout;     /* raw timeout ticks */
    bool     dynamicWait;        /* adaptive delay enabled */
    bool     permanentWait;      /* infinite wait enabled */
    uint32_t waitStartTime;     /* timestamp of wait start */
    uint16_t failureCount;       /* retry attempts */
    uint16_t bootError;          /* last boot error code */
    uint32_t sysResHandle;       /* System file resource map */
    uint32_t blessedFolderID;    /* dirID of blessed folder */
} LC3BootStrategy;

/* Drive Queue Header Entry */
typedef struct LC3DrvQEl {
    struct LC3DrvQEl *qLink;
    uint16_t          qType;
    uint16_t          dQDrive;
    int16_t           dQRefNum;
    uint16_t          dQFSID;
    uint16_t          dQDrvSz;
    uint16_t          dQDrvSz2;
} LC3DrvQEl;

/* Low-memory global addresses */
#define LM_BOOTDRIVE        (*(volatile uint16_t *)0x0210)
#define LM_BTDSKRFN         (*(volatile int16_t  *)0x0B34)
#define LM_DRVQHDR          (*(volatile LC3DrvQEl **)0x030A)
#define LM_BUFPTR           (*(volatile uint32_t *)0x010C)
#define LM_MEMTOP           (*(volatile uint32_t *)0x0108)
#define LM_CRSRSYS          (*(volatile uint8_t  *)0x08CD)
#define LM_TICKS            (*(volatile uint32_t *)0x016A)
#define LM_BOOTGLOB         (*(volatile uint32_t *)0x0B5C)

/* Boot Strategy API */
void     lc3_boot_init_jump_table(void);
void     lc3_boot_fill_with_ones(void *ptr, uint32_t bytes);
void     lc3_boot_draw_beep_screen(uint8_t alertLevel);
void     lc3_boot_init_himem_globals(uint32_t ramTop);
uint32_t lc3_boot_get_pram_time(void);
void     lc3_boot_plan_strategy(LC3BootStrategy *strat);
LC3DrvQEl *lc3_boot_next_dq_entry(LC3DrvQEl *current);
bool     lc3_boot_select_device(LC3BootStrategy *strat, uint16_t drive);
bool     lc3_boot_check_mouse_eject(void);
void     lc3_boot_get_startup_info(LC3BootStrategy *strat);
void     lc3_boot_react_to_failure(LC3BootStrategy *strat);
void     lc3_boot_visual_update(bool showHappyMac);
void     lc3_boot_show_success(uint16_t driveNum);
void     lc3_boot_eject_me(uint16_t driveNum);
bool     lc3_boot_is_it_floppy(uint16_t driveNum);
bool     lc3_boot_in_boot_mask_chk(LC3BootStrategy *strat, uint16_t maskBit);
void     lc3_boot_set_timeout_merge(LC3BootStrategy *strat, uint32_t timeout);
void     lc3_boot_disable_dyn_wait(LC3BootStrategy *strat);
void     lc3_boot_enable_dyn_wait(LC3BootStrategy *strat);
void     lc3_boot_disable_perm_wait(LC3BootStrategy *strat);
void     lc3_boot_enable_perm_wait(LC3BootStrategy *strat);
uint32_t lc3_boot_get_raw_timeout(const LC3BootStrategy *strat);
void     lc3_boot_set_raw_timeout(LC3BootStrategy *strat, uint32_t timeout);
int16_t  lc3_boot_open_system_file(const char *name, uint32_t dirID);
int16_t  lc3_boot_check_file_exists(int16_t vRefNum, const char *name);
void    *lc3_boot_alloc_boot_buf(uint32_t bytes);
void    *lc3_boot_alloc_sys_ptr(uint32_t bytes);
int16_t  lc3_boot_set_vol_blessed_folder(int16_t vRefNum, uint32_t dirID);
void     lc3_boot_launch_app(const char *appName);

#endif /* LC3_BOOT_MGR_H */
