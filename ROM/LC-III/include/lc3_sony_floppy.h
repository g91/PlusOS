/* lc3_sony_floppy.h - Macintosh LC III Sony SuperDrive (SWIM) Floppy Disk Driver
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x006D4F - 0x009950
 */
#ifndef LC3_SONY_FLOPPY_H
#define LC3_SONY_FLOPPY_H

#include <stdint.h>
#include <stdbool.h>

#define SONY_DRIVE_INTERNAL  1
#define SONY_DRIVE_EXTERNAL  2

#define SONY_DENSITY_GCR_400K 1
#define SONY_DENSITY_GCR_800K 2
#define SONY_DENSITY_MFM_144M 3

/* Sony Driver Param Block */
typedef struct {
    int16_t  ioVRefNum;
    int16_t  ioRefNum;
    int16_t  csCode;
    uint32_t csParam[11];
} LC3SonyParamBlock;

/* Sony Driver Interface */
int16_t  lc3_sony_open(int16_t drvrRefNum);
int16_t  lc3_sony_close(int16_t drvrRefNum);
int16_t  lc3_sony_read(uint8_t drive, uint32_t sector, void *buffer, uint16_t sectorCount);
int16_t  lc3_sony_write(uint8_t drive, uint32_t sector, const void *buffer, uint16_t sectorCount);
int16_t  lc3_sony_control(LC3SonyParamBlock *pb);
int16_t  lc3_sony_status(LC3SonyParamBlock *pb);
int16_t  lc3_sony_eject(uint8_t drive);
uint8_t  lc3_sony_detect_density(uint8_t drive);
bool     lc3_sony_is_disk_inserted(uint8_t drive);
bool     lc3_sony_is_write_protected(uint8_t drive);

#endif /* LC3_SONY_FLOPPY_H */
