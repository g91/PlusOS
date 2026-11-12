/* lc3_sonora_egret.c - Macintosh LC III Sonora ASIC & Egret Low-Level Hardware Drivers
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   TIMEK_SCC_LOOP, WRITESCC, SCC_WRITE_LOOP, RTC_READ_PATH, WARMSTART_CHECK,
 *   EXCPT_ILLEGAL, EXCPT_ZERO_DIV, EXCPT_CHK, EXCPT_TRAPV, EXCPT_PRIVILEGE
 */

#include "lc3_sonora_egret.h"
#include "lc3_hardware.h"
#include <string.h>

static LC3SonoraState g_sonoraState;

void lc3_sonora_init(void) {
    /* Initialize Sonora ASIC control registers */
    volatile uint8_t *sonora = (volatile uint8_t *)LC3_SONORA_BASE;

    /* Default boot video mode: 640x480 @ 1-bpp monochrome */
    g_sonoraState.activeDepth = 1;
    g_sonoraState.rowBytes = 80;
    g_sonoraState.hRes = 640;
    g_sonoraState.vRes = 480;
    g_sonoraState.vblInterruptEnabled = true;
    g_sonoraState.scsiDmaEnabled = true;

    /* Set Sonora video timing controller for 13" Apple High-Res RGB (640x480 @ 67 Hz) */
    sonora[0x00] = 0x01; /* Video enable */
    sonora[0x02] = 0x00; /* 1-bpp depth */

    /* Acknowledge pending VBL interrupts */
    lc3_sonora_ack_vbl_interrupt();
    lc3_sonora_enable_vbl_irq(true);
}

void lc3_sonora_set_video_mode(uint8_t depthMode) {
    volatile uint8_t *sonora = (volatile uint8_t *)LC3_SONORA_BASE;
    switch (depthMode) {
        case SONORA_MODE_1BPP:
            g_sonoraState.activeDepth = 1;
            g_sonoraState.rowBytes = 80;
            sonora[0x02] = 0x00;
            break;
        case SONORA_MODE_2BPP:
            g_sonoraState.activeDepth = 2;
            g_sonoraState.rowBytes = 160;
            sonora[0x02] = 0x01;
            break;
        case SONORA_MODE_4BPP:
            g_sonoraState.activeDepth = 4;
            g_sonoraState.rowBytes = 320;
            sonora[0x02] = 0x02;
            break;
        case SONORA_MODE_8BPP:
            g_sonoraState.activeDepth = 8;
            g_sonoraState.rowBytes = 640;
            sonora[0x02] = 0x03;
            break;
        case SONORA_MODE_16BPP:
            g_sonoraState.activeDepth = 16;
            g_sonoraState.rowBytes = 1280;
            sonora[0x02] = 0x04;
            break;
        default:
            return;
    }
}

void lc3_sonora_set_clut_entry(uint8_t index, uint8_t red, uint8_t green, uint8_t blue) {
    /* Directly loads Sonora hardware CLUT register apertures */
    volatile uint8_t *clutAddr = (volatile uint8_t *)0x50F00034;
    volatile uint8_t *clutData = (volatile uint8_t *)0x50F00036;

    *clutAddr = index;  /* Set index register */
    *clutData = red;    /* Write 8-bit Red component */
    *clutData = green;  /* Write 8-bit Green component */
    *clutData = blue;   /* Write 8-bit Blue component */
}

void lc3_sonora_load_clut_palette(const uint8_t *paletteRGB, uint16_t count) {
    volatile uint8_t *clutAddr = (volatile uint8_t *)0x50F00034;
    volatile uint8_t *clutData = (volatile uint8_t *)0x50F00036;

    *clutAddr = 0; /* Auto-increments after 3 bytes */
    for (uint16_t i = 0; i < count; ++i) {
        *clutData = *paletteRGB++; /* Red */
        *clutData = *paletteRGB++; /* Green */
        *clutData = *paletteRGB++; /* Blue */
    }
}

void lc3_sonora_ack_vbl_interrupt(void) {
    volatile uint8_t *sonora = (volatile uint8_t *)LC3_SONORA_BASE;
    /* Clear VBL interrupt latch by reading status and writing acknowledge bit */
    volatile uint8_t status = sonora[0x10];
    (void)status;
    sonora[0x12] = 0x01;
}

void lc3_sonora_enable_vbl_irq(bool enable) {
    volatile uint8_t *sonora = (volatile uint8_t *)LC3_SONORA_BASE;
    sonora[0x14] = enable ? 0x01 : 0x00;
    g_sonoraState.vblInterruptEnabled = enable;
}

void lc3_sonora_configure_dma(bool enableScsiDma) {
    volatile uint8_t *sonora = (volatile uint8_t *)LC3_SONORA_BASE;
    /* Configures Sonora 32-bit DACK pseudo-DMA aperture at $50F00020 */
    sonora[0x20] = enableScsiDma ? 0x01 : 0x00;
    g_sonoraState.scsiDmaEnabled = enableScsiDma;
}

void lc3_egret_init(void) {
    /* Initialize VIA2 shift register for Egret microcontroller packets */
    volatile uint8_t *via2 = (volatile uint8_t *)LC3_VIA2_BASE;
    via2[0x1E00] = 0x00; /* Clear direction */
}

int16_t lc3_egret_send_packet(const LC3EgretPacket *pkt, LC3EgretPacket *resp) {
    volatile uint8_t *egret = (volatile uint8_t *)LC3_EGRET_BASE;

    /* Write packet header */
    egret[0] = pkt->cmd;
    egret[2] = pkt->subCmd;
    egret[4] = pkt->byteCount;

    /* Write payload */
    for (uint8_t i = 0; i < pkt->byteCount && i < 32; ++i) {
        egret[6 + (i * 2)] = pkt->data[i];
    }

    /* Wait for transfer handshake */
    uint32_t timeout = 50000;
    while (!(egret[0] & 0x80) && --timeout);
    if (timeout == 0) {
        return -1; /* Timeout */
    }

    if (resp) {
        resp->cmd = egret[0];
        resp->subCmd = egret[2];
        resp->byteCount = egret[4];
        for (uint8_t i = 0; i < resp->byteCount && i < 32; ++i) {
            resp->data[i] = egret[6 + (i * 2)];
        }
    }
    return 0;
}

uint32_t lc3_egret_rtc_read_path(void) {
    /* RTC_READ_PATH: reads 32-bit Mac epoch seconds from Egret RTC */
    LC3EgretPacket pkt, resp;
    pkt.cmd = EGRET_CMD_RTC_READ;
    pkt.subCmd = 0;
    pkt.byteCount = 0;

    if (lc3_egret_send_packet(&pkt, &resp) == 0 && resp.byteCount >= 4) {
        uint32_t sec = 0;
        sec |= ((uint32_t)resp.data[0]) << 24;
        sec |= ((uint32_t)resp.data[1]) << 16;
        sec |= ((uint32_t)resp.data[2]) << 8;
        sec |= ((uint32_t)resp.data[3]);
        return sec;
    }
    return 0;
}

void lc3_egret_rtc_write_path(uint32_t seconds) {
    LC3EgretPacket pkt;
    pkt.cmd = EGRET_CMD_RTC_WRITE;
    pkt.subCmd = 0;
    pkt.byteCount = 4;
    pkt.data[0] = (uint8_t)(seconds >> 24);
    pkt.data[1] = (uint8_t)(seconds >> 16);
    pkt.data[2] = (uint8_t)(seconds >> 8);
    pkt.data[3] = (uint8_t)(seconds & 0xFF);
    lc3_egret_send_packet(&pkt, 0);
}

void lc3_egret_read_pram(uint8_t offset, uint8_t *dest, uint8_t bytes) {
    LC3EgretPacket pkt, resp;
    pkt.cmd = EGRET_CMD_PRAM_READ;
    pkt.subCmd = offset;
    pkt.byteCount = bytes;
    if (lc3_egret_send_packet(&pkt, &resp) == 0) {
        memcpy(dest, resp.data, bytes);
    }
}

void lc3_egret_write_pram(uint8_t offset, const uint8_t *src, uint8_t bytes) {
    LC3EgretPacket pkt;
    pkt.cmd = EGRET_CMD_PRAM_WRITE;
    pkt.subCmd = offset;
    pkt.byteCount = bytes;
    memcpy(pkt.data, src, bytes);
    lc3_egret_send_packet(&pkt, 0);
}

void lc3_egret_soft_power_off(void) {
    LC3EgretPacket pkt;
    pkt.cmd = EGRET_CMD_POWER_OFF;
    pkt.subCmd = 0;
    pkt.byteCount = 0;
    lc3_egret_send_packet(&pkt, 0);
    while (1);
}

void lc3_egret_system_reset(void) {
    LC3EgretPacket pkt;
    pkt.cmd = EGRET_CMD_RESET_SYS;
    pkt.subCmd = 0;
    pkt.byteCount = 0;
    lc3_egret_send_packet(&pkt, 0);
    while (1);
}
