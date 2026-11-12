/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: CommToolbox File Transfer Manager Core
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_CTB_FT_H
#define LC3_CTB_FT_H

#include <stdint.h>
#include <stdbool.h>

#define FT_NO_ERR           0
#define FT_PARAM_ERR      -50
#define FT_ABORTED        -51
#define FT_CHECKSUM_ERR   -52
#define FT_SEQ_ERR        -53

/* XMODEM Control Characters */
#define XMODEM_SOH          0x01
#define XMODEM_STX          0x02
#define XMODEM_EOT          0x04
#define XMODEM_ACK          0x06
#define XMODEM_NAK          0x15
#define XMODEM_CAN          0x18

typedef struct FTRecord {
    int16_t procID;             /* 1 = XMODEM, 2 = YMODEM, 3 = ZMODEM */
    bool isSending;
    uint32_t currentPacketNum;
    uint32_t totalBytesTransferred;
    bool useCRC;
    bool aborted;
} FTRecord, *FTRecordPtr, **FTHandle;

/* Public API */
int16_t lc3_ctb_ft_init(void);
int16_t lc3_ctb_ft_start_send(FTHandle hFT, int16_t toolID, bool useCRC);
int16_t lc3_ctb_ft_start_receive(FTHandle hFT, int16_t toolID, bool useCRC);
int16_t lc3_ctb_ft_send_packet_xmodem(FTHandle hFT, const uint8_t *payload128, uint8_t *outPacket132);
int16_t lc3_ctb_ft_receive_packet_xmodem(FTHandle hFT, const uint8_t *inPacket132, uint8_t *payload128);
int16_t lc3_ctb_ft_abort(FTHandle hFT, uint8_t *outCanBytes, uint16_t *outLen);

#endif /* LC3_CTB_FT_H */
