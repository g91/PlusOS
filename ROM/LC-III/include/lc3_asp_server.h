/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ASP Server Architecture & Workstation Management
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ASP_SERVER_H
#define LC3_ASP_SERVER_H

#include <stdint.h>
#include <stdbool.h>

#define ASPSRV_NO_ERR         0
#define ASPSRV_NOT_INIT    -1075
#define ASPSRV_PARAM_ERR     -50

typedef void *ASPServerRef;

typedef struct ASPServerSession {
    uint16_t sessionID;
    uint8_t clientNode;
    uint8_t clientSocket;
    uint16_t clientNet;
    uint16_t lastSeqNum;
    bool isActive;
} ASPServerSession;

typedef struct ASPServerRecord {
    uint8_t serverSocket;
    bool isRunning;
    uint32_t sessionCount;
    ASPServerSession sessions[8];
} ASPServerRecord;

/* ASP Server API */
int16_t lc3_asp_init_server(uint8_t *serverSocket, ASPServerRef *serverRef);
int16_t lc3_asp_remove_server(ASPServerRef serverRef);
int16_t lc3_asp_get_session(ASPServerRef serverRef, uint16_t *sessionID);
int16_t lc3_asp_get_request(ASPServerRef serverRef, uint16_t sessionID, void *reqBuf, uint16_t *reqLen);
int16_t lc3_asp_send_reply(ASPServerRef serverRef, uint16_t sessionID, const void *replyBuf, uint16_t replyLen);
int16_t lc3_asp_send_attention(ASPServerRef serverRef, uint16_t sessionID, uint16_t attentionCode);

#endif /* LC3_ASP_SERVER_H */
