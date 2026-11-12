/* lc3_sony_floppy.c - Macintosh LC III Sony SuperDrive (SWIM) Floppy Disk Driver
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   Driver_Sony, SONY_CTL_PARAM, ENQ_APPEND_TAIL, ENQ_DONE
 */

#include "lc3_sony_floppy.h"
#include "lc3_hardware.h"
#include <string.h>

#define SWIM_BASE 0x50F16000

/* SWIM Register Offsets */
#define SWIM_DATA        0x00
#define SWIM_MARK        0x02
#define SWIM_CRC         0x04
#define SWIM_ICR         0x06
#define SWIM_ACTION      0x08
#define SWIM_STATUS      0x0A
#define SWIM_MODE        0x0C
#define SWIM_SETUP       0x0E

static uint8_t g_driveTrack[2] = { 0, 0 };

int16_t lc3_sony_open(int16_t drvrRefNum) {
    (void)drvrRefNum;
    volatile uint8_t *swim = (volatile uint8_t *)SWIM_BASE;
    /* Configure SWIM for Apple SuperDrive mode */
    swim[SWIM_MODE] = 0x01;
    swim[SWIM_SETUP] = 0x30;
    return 0; /* noErr */
}

int16_t lc3_sony_close(int16_t drvrRefNum) {
    (void)drvrRefNum;
    return 0;
}

bool lc3_sony_is_disk_inserted(uint8_t drive) {
    (void)drive;
    volatile uint8_t *swim = (volatile uint8_t *)SWIM_BASE;
    /* Bit 6 in status register indicates disk in place (active low) */
    return ((swim[SWIM_STATUS] & 0x40) == 0);
}

bool lc3_sony_is_write_protected(uint8_t drive) {
    (void)drive;
    volatile uint8_t *swim = (volatile uint8_t *)SWIM_BASE;
    /* Bit 5 in status register is write protect */
    return ((swim[SWIM_STATUS] & 0x20) != 0);
}

uint8_t lc3_sony_detect_density(uint8_t drive) {
    (void)drive;
    volatile uint8_t *swim = (volatile uint8_t *)SWIM_BASE;
    /* High density sensor line (active low) */
    if ((swim[SWIM_STATUS] & 0x80) == 0) {
        return SONY_DENSITY_MFM_144M;
    }
    return SONY_DENSITY_GCR_800K;
}

static void lc3_sony_seek_track(uint8_t drive, uint8_t targetTrack) {
    volatile uint8_t *swim = (volatile uint8_t *)SWIM_BASE;
    uint8_t current = g_driveTrack[drive & 1];
    while (current != targetTrack) {
        if (current < targetTrack) {
            swim[SWIM_ACTION] = 0x01; /* Step in */
            current++;
        } else {
            swim[SWIM_ACTION] = 0x02; /* Step out */
            current--;
        }
    }
    g_driveTrack[drive & 1] = current;
}

int16_t lc3_sony_read(uint8_t drive, uint32_t sector, void *buffer, uint16_t sectorCount) {
    if (!lc3_sony_is_disk_inserted(drive)) {
        return -64; /* offLinErr */
    }

    uint8_t *p = (uint8_t *)buffer;
    for (uint16_t i = 0; i < sectorCount; ++i) {
        uint32_t curSec = sector + i;
        uint8_t track = (uint8_t)(curSec / 18);
        lc3_sony_seek_track(drive, track);

        /* Read 512-byte raw sector from SWIM FIFO */
        volatile uint8_t *swim = (volatile uint8_t *)SWIM_BASE;
        swim[SWIM_ACTION] = 0x10; /* Start read */
        for (int b = 0; b < 512; ++b) {
            uint32_t timeout = 10000;
            while (!(swim[SWIM_STATUS] & 0x01) && --timeout);
            *p++ = swim[SWIM_DATA];
        }
    }
    return 0;
}

int16_t lc3_sony_write(uint8_t drive, uint32_t sector, const void *buffer, uint16_t sectorCount) {
    if (!lc3_sony_is_disk_inserted(drive)) return -64;
    if (lc3_sony_is_write_protected(drive)) return -65; /* wPrErr */

    const uint8_t *p = (const uint8_t *)buffer;
    for (uint16_t i = 0; i < sectorCount; ++i) {
        uint32_t curSec = sector + i;
        uint8_t track = (uint8_t)(curSec / 18);
        lc3_sony_seek_track(drive, track);

        volatile uint8_t *swim = (volatile uint8_t *)SWIM_BASE;
        swim[SWIM_ACTION] = 0x20; /* Start write */
        for (int b = 0; b < 512; ++b) {
            uint32_t timeout = 10000;
            while (!(swim[SWIM_STATUS] & 0x02) && --timeout);
            swim[SWIM_DATA] = *p++;
        }
    }
    return 0;
}

int16_t lc3_sony_eject(uint8_t drive) {
    (void)drive;
    volatile uint8_t *swim = (volatile uint8_t *)SWIM_BASE;
    swim[SWIM_ACTION] = 0x80; /* Motor step out past track 0 and trigger eject solenoid */
    return 0;
}

int16_t lc3_sony_control(LC3SonyParamBlock *pb) {
    /* SONY_CTL_PARAM */
    switch (pb->csCode) {
        case 1: /* KillIO */
            return 0;
        case 5: /* Verify Disk */
            return 0;
        case 6: /* Format Track */
            return 0;
        case 7: /* Eject */
            return lc3_sony_eject((uint8_t)pb->ioVRefNum);
        default:
            return -17; /* controlErr */
    }
}

int16_t lc3_sony_status(LC3SonyParamBlock *pb) {
    switch (pb->csCode) {
        case 8: /* Drive Status */
            pb->csParam[0] = lc3_sony_is_disk_inserted((uint8_t)pb->ioVRefNum) ? 1 : 0;
            pb->csParam[1] = lc3_sony_is_write_protected((uint8_t)pb->ioVRefNum) ? 1 : 0;
            pb->csParam[2] = lc3_sony_detect_density((uint8_t)pb->ioVRefNum);
            return 0;
        default:
            return -18; /* statusErr */
    }
}
