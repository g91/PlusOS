/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: CommToolbox File Transfer Manager Core
 *
 * Implements XMODEM-checksum and XMODEM-CRC packet framing, verification,
 * sequence tracking, and transfer abort sequences with zero stubs.
 */

#include "lc3_ctb_ft.h"

static bool s_ftInitialized = false;

/* CRC-CCITT (0x1021) calculator */
static uint16_t calc_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0x0000;
    for (uint16_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/* 1470: lc3_ctb_ft_init */
int16_t lc3_ctb_ft_init(void)
{
    s_ftInitialized = true;
    return FT_NO_ERR;
}

/* 1471: lc3_ctb_ft_start_send */
int16_t lc3_ctb_ft_start_send(FTHandle hFT, int16_t toolID, bool useCRC)
{
    if (!hFT || !*hFT) return FT_PARAM_ERR;
    if (!s_ftInitialized) lc3_ctb_ft_init();

    FTRecordPtr ft = *hFT;
    ft->procID = toolID;
    ft->isSending = true;
    ft->currentPacketNum = 1;
    ft->totalBytesTransferred = 0;
    ft->useCRC = useCRC;
    ft->aborted = false;

    return FT_NO_ERR;
}

/* 1472: lc3_ctb_ft_start_receive */
int16_t lc3_ctb_ft_start_receive(FTHandle hFT, int16_t toolID, bool useCRC)
{
    if (!hFT || !*hFT) return FT_PARAM_ERR;
    if (!s_ftInitialized) lc3_ctb_ft_init();

    FTRecordPtr ft = *hFT;
    ft->procID = toolID;
    ft->isSending = false;
    ft->currentPacketNum = 1;
    ft->totalBytesTransferred = 0;
    ft->useCRC = useCRC;
    ft->aborted = false;

    return FT_NO_ERR;
}

/* 1473: lc3_ctb_ft_send_packet_xmodem */
int16_t lc3_ctb_ft_send_packet_xmodem(FTHandle hFT, const uint8_t *payload128, uint8_t *outPacket132)
{
    if (!hFT || !*hFT || !payload128 || !outPacket132) return FT_PARAM_ERR;

    FTRecordPtr ft = *hFT;
    if (ft->aborted) return FT_ABORTED;

    uint8_t pktNum = (uint8_t)(ft->currentPacketNum & 0xFF);
    uint8_t pktNumInv = (uint8_t)(~pktNum);

    outPacket132[0] = XMODEM_SOH;
    outPacket132[1] = pktNum;
    outPacket132[2] = pktNumInv;

    for (int i = 0; i < 128; ++i) {
        outPacket132[3 + i] = payload128[i];
    }

    if (ft->useCRC) {
        uint16_t crc = calc_crc16(payload128, 128);
        outPacket132[131] = (uint8_t)((crc >> 8) & 0xFF);
        outPacket132[132] = (uint8_t)(crc & 0xFF);
    } else {
        uint8_t sum = 0;
        for (int i = 0; i < 128; ++i) {
            sum += payload128[i];
        }
        outPacket132[131] = sum;
    }

    ft->currentPacketNum++;
    ft->totalBytesTransferred += 128;

    return FT_NO_ERR;
}

/* 1474: lc3_ctb_ft_receive_packet_xmodem */
int16_t lc3_ctb_ft_receive_packet_xmodem(FTHandle hFT, const uint8_t *inPacket132, uint8_t *payload128)
{
    if (!hFT || !*hFT || !inPacket132 || !payload128) return FT_PARAM_ERR;

    FTRecordPtr ft = *hFT;
    if (ft->aborted) return FT_ABORTED;

    if (inPacket132[0] != XMODEM_SOH && inPacket132[0] != XMODEM_STX) {
        return FT_PARAM_ERR;
    }

    uint8_t pktNum = inPacket132[1];
    uint8_t pktNumInv = inPacket132[2];

    if ((uint8_t)(pktNum + pktNumInv) != 0xFF) {
        return FT_SEQ_ERR;
    }

    /* Verify checksum / CRC */
    if (ft->useCRC) {
        uint16_t receivedCrc = ((uint16_t)inPacket132[131] << 8) | (uint16_t)inPacket132[132];
        uint16_t computedCrc = calc_crc16(&inPacket132[3], 128);
        if (receivedCrc != computedCrc) {
            return FT_CHECKSUM_ERR;
        }
    } else {
        uint8_t sum = 0;
        for (int i = 0; i < 128; ++i) {
            sum += inPacket132[3 + i];
        }
        if (sum != inPacket132[131]) {
            return FT_CHECKSUM_ERR;
        }
    }

    /* Copy validated payload */
    for (int i = 0; i < 128; ++i) {
        payload128[i] = inPacket132[3 + i];
    }

    ft->currentPacketNum++;
    ft->totalBytesTransferred += 128;

    return FT_NO_ERR;
}

/* 1475: lc3_ctb_ft_abort */
int16_t lc3_ctb_ft_abort(FTHandle hFT, uint8_t *outCanBytes, uint16_t *outLen)
{
    if (!hFT || !*hFT || !outCanBytes || !outLen) return FT_PARAM_ERR;

    FTRecordPtr ft = *hFT;
    ft->aborted = true;

    /* Transmit standard cancel stream: 5 x CAN (0x18) */
    for (int i = 0; i < 5; ++i) {
        outCanBytes[i] = XMODEM_CAN;
    }
    *outLen = 5;

    return FT_NO_ERR;
}
