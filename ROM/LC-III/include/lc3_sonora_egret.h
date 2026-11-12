/* lc3_sonora_egret.h - Macintosh LC III Sonora ASIC & Egret Low-Level Hardware Drivers
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x000A12 - 0x004B26
 */
#ifndef LC3_SONORA_EGRET_H
#define LC3_SONORA_EGRET_H

#include <stdint.h>
#include <stdbool.h>

/* Sonora ASIC Video Modes */
#define SONORA_MODE_1BPP     0x00
#define SONORA_MODE_2BPP     0x01
#define SONORA_MODE_4BPP     0x02
#define SONORA_MODE_8BPP     0x03
#define SONORA_MODE_16BPP    0x04

/* Egret ADB Commands */
#define EGRET_CMD_ADB_SEND   0x00
#define EGRET_CMD_RTC_READ   0x01
#define EGRET_CMD_RTC_WRITE  0x02
#define EGRET_CMD_PRAM_READ  0x03
#define EGRET_CMD_PRAM_WRITE 0x04
#define EGRET_CMD_POWER_OFF  0x07
#define EGRET_CMD_RESET_SYS  0x08

/* Sonora Hardware State */
typedef struct {
    uint8_t  activeDepth;       /* 1, 2, 4, 8, or 16-bpp */
    uint16_t rowBytes;          /* scanline byte width */
    uint16_t hRes;              /* 640 */
    uint16_t vRes;              /* 480 */
    bool     vblInterruptEnabled;
    bool     scsiDmaEnabled;
} LC3SonoraState;

/* Egret ADB Packet Header */
typedef struct {
    uint8_t  cmd;
    uint8_t  subCmd;
    uint8_t  byteCount;
    uint8_t  data[32];
} LC3EgretPacket;

/* Sonora Low-Level API */
void     lc3_sonora_init(void);
void     lc3_sonora_set_video_mode(uint8_t depthMode);
void     lc3_sonora_set_clut_entry(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void     lc3_sonora_load_clut_palette(const uint8_t *paletteRGB, uint16_t count);
void     lc3_sonora_ack_vbl_interrupt(void);
void     lc3_sonora_enable_vbl_irq(bool enable);
void     lc3_sonora_configure_dma(bool enableScsiDma);

/* Egret Microcontroller Low-Level API */
void     lc3_egret_init(void);
int16_t  lc3_egret_send_packet(const LC3EgretPacket *pkt, LC3EgretPacket *resp);
uint32_t lc3_egret_rtc_read_path(void);
void     lc3_egret_rtc_write_path(uint32_t seconds);
void     lc3_egret_read_pram(uint8_t offset, uint8_t *dest, uint8_t bytes);
void     lc3_egret_write_pram(uint8_t offset, const uint8_t *src, uint8_t bytes);
void     lc3_egret_soft_power_off(void);
void     lc3_egret_system_reset(void);

#endif /* LC3_SONORA_EGRET_H */
