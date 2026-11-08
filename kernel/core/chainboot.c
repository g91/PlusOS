/* chainboot.c - start another operating system from a chosen SCSI device.
 *
 * On the Mac SE the ROM's startup search reads the default startup device from
 * extended PRAM (XPRAM offset $78: a 4-byte record = {drive number, driver
 * refNum}). We set that record and restart through the ROM, which then boots
 * that disk with its own, fully-tested boot path - the same thing the Startup
 * Disk control panel does. The Mac Plus ROM has no such search (it always boots
 * the highest SCSI ID), so chain-boot only works on the SE.
 *
 * We do NOT guess the SCSI-ID -> refNum mapping. We walk the drive queue and
 * match the entry whose driver refNum is -(33 + ID) (the Mac convention:
 * ID0 = 0xFFDF .. ID7 = 0xFFD8), then write that entry's exact refNum and drive
 * number. If no such drive exists we report it instead of booting the wrong one.
 *
 * Drive queue element (from the qHead pointer): dQDrive @ +6, dQRefNum @ +8.
 */
#include "kernel.h"

static void xpram_write(uint32_t ctl, const void *buf)
{
    register uint32_t d0 __asm__("d0") = ctl;
    register const void *a0 __asm__("a0") = buf;
    __asm__ volatile(".short 0xA052" : "+d"(d0), "+a"(a0) : : "d1", "d2", "a1", "cc", "memory");
}

static void xpram_read(uint32_t ctl, void *buf)
{
    register uint32_t d0 __asm__("d0") = ctl;
    register void *a0 __asm__("a0") = buf;
    __asm__ volatile(".short 0xA051" : "+d"(d0), "+a"(a0) : : "d1", "d2", "a1", "cc", "memory");
}

#define XPRAM_STARTUP_CTL  0x00040078u      /* 4 bytes at XPRAM offset $78 */

static uint16_t refnum_for_id(int id) { return (uint16_t)(0xFFDF - id); }   /* -(33+id) */

/* Walk the drive queue; call fn(refnum, drive) for each entry. */
void chainboot_each_drive(void (*fn)(int refnum, int drive))
{
    uint8_t *q = (uint8_t *)LM_LONG(0x30A);     /* DrvQHdr.qHead */
    while (q) {
        int refnum = *(int16_t *)(q + 8);
        int drive  = *(int16_t *)(q + 6);
        fn(refnum, drive);
        q = *(uint8_t **)q;                     /* qLink */
    }
}

/* Find the drive-queue entry for this SCSI ID. Returns its drive number and
 * refNum through the pointers, or 0 if not present. */
static int find_scsi(int id, uint16_t *refnum_out, uint16_t *drive_out)
{
    uint16_t want = refnum_for_id(id);
    for (uint8_t *q = (uint8_t *)LM_LONG(0x30A); q; q = *(uint8_t **)q) {
        uint16_t refnum = (uint16_t)*(int16_t *)(q + 8);
        if (refnum == want) {
            *refnum_out = refnum;
            *drive_out = (uint16_t)*(int16_t *)(q + 6);
            return 1;
        }
    }
    return 0;
}

/* Set the SE default startup device to this SCSI ID (no reboot).
 * Returns 0 on success, -1 if not an SE, -2 if no such drive is attached. */
int chainboot_set_default(int scsi_id)
{
    if (!machine_is_se()) return -1;
    if (scsi_id < 0 || scsi_id > 7) return -1;
    uint16_t refnum, drive;
    if (!find_scsi(scsi_id, &refnum, &drive)) return -2;
    uint8_t rec[4];
    rec[0] = (uint8_t)(drive >> 8); rec[1] = (uint8_t)drive;
    rec[2] = (uint8_t)(refnum >> 8); rec[3] = (uint8_t)refnum;
    xpram_write(XPRAM_STARTUP_CTL, rec);
    return 0;
}

/* The SCSI ID the SE would boot by default, or -1 if none/not SE. */
int chainboot_get_default(void)
{
    if (!machine_is_se()) return -1;
    uint8_t rec[4];
    xpram_read(XPRAM_STARTUP_CTL, rec);
    uint16_t refnum = (uint16_t)((rec[2] << 8) | rec[3]);
    if (refnum >= 0xFFD8 && refnum <= 0xFFDF) return 0xFFDF - refnum;
    return -1;
}

/* Boot the OS on this SCSI ID now: set the default device and restart.
 * Returns <0 (does not reboot) on failure so the caller can explain. */
int chainboot(int scsi_id)
{
    int rc = chainboot_set_default(scsi_id);
    if (rc < 0) return rc;
    sys_reboot();                               /* ROM boots the chosen disk */
    return 0;                                    /* unreachable */
}
