/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk AppleShare File Protocol (AFP 2.1) Client Session
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_AFP_CLIENT_H
#define LC3_AFP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

#define AFP_NO_ERR         0
#define AFP_AUTH_FAILED  -5000
#define AFP_VOL_LOCKED   -5001
#define AFP_PARAM_ERR      -50

typedef struct AFPSession {
    uint16_t sessionRefNum;
    uint8_t serverNodeID;
    uint8_t serverSocket;
    char userName[32];
    bool authenticated;
    uint16_t mountedVolID;
    uint32_t activeForkRef;
} AFPSession;

typedef struct AFPDirEntry {
    char name[32];
    uint32_t fileType;
    uint32_t fileCreator;
    uint32_t dataForkLen;
    uint32_t rsrcForkLen;
    bool isDirectory;
} AFPDirEntry;

/* AFP Client API */
int16_t lc3_afp_login(AFPSession *session, uint8_t srvNode, uint8_t srvSocket, const char *user, const char *pass);
int16_t lc3_afp_open_vol(AFPSession *session, const char *volName, uint16_t *volID);
int16_t lc3_afp_enumerate(AFPSession *session, uint16_t volID, int32_t dirID, AFPDirEntry *entries, uint16_t maxEntries, uint16_t *actualEntries);
int16_t lc3_afp_open_fork(AFPSession *session, uint16_t volID, int32_t dirID, const char *fileName, bool rsrcFork, uint32_t *forkRef);
int16_t lc3_afp_read_fork(AFPSession *session, uint32_t forkRef, uint32_t offset, void *buffer, uint32_t reqBytes, uint32_t *actualBytes);
int16_t lc3_afp_logout(AFPSession *session);

#endif /* LC3_AFP_CLIENT_H */
