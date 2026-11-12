/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ASP (Session Protocol) & PAP (Printer Access Protocol)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_APPLETALK_ASP_PAP_H
#define LC3_APPLETALK_ASP_PAP_H

#include <stdint.h>
#include <stdbool.h>

#define ASP_NO_ERR         0
#define ASP_SERVER_BUSY -1071
#define ASP_PARAM_ERR     -50

typedef uint16_t ASPSessionRef;
typedef uint16_t PAPSessionRef;

typedef struct ASPSessionRecord {
    ASPSessionRef sessRef;
    uint8_t serverNode;
    uint8_t serverSocket;
    uint16_t serverNet;
    uint16_t sequenceNum;
    bool isOpen;
} ASPSessionRecord;

typedef struct PAPSessionRecord {
    PAPSessionRef sessRef;
    uint8_t printerNode;
    uint8_t printerSocket;
    uint16_t printerNet;
    bool isOpen;
    char printerStatus[128];
} PAPSessionRecord;

/* ASP API */
int16_t lc3_asp_open_session(uint16_t net, uint8_t node, uint8_t socket, ASPSessionRef *sessRef);
int16_t lc3_asp_close_session(ASPSessionRef sessRef);
int16_t lc3_asp_user_command(ASPSessionRef sessRef, const void *cmdBuf, uint16_t cmdLen, void *replyBuf, uint16_t *replyLen);
int16_t lc3_asp_user_write(ASPSessionRef sessRef, const void *cmdBuf, uint16_t cmdLen, const void *writeBuf, uint32_t writeLen);

/* PAP API */
int16_t lc3_pap_open(uint16_t net, uint8_t node, uint8_t socket, PAPSessionRef *sessRef);
int16_t lc3_pap_close(PAPSessionRef sessRef);
int16_t lc3_pap_read(PAPSessionRef sessRef, void *dataBuf, uint32_t *readLen, bool *eof);
int16_t lc3_pap_write(PAPSessionRef sessRef, const void *dataBuf, uint32_t writeLen, bool eof);
int16_t lc3_pap_status(uint16_t net, uint8_t node, uint8_t socket, char *statusStr);

#endif /* LC3_APPLETALK_ASP_PAP_H */
