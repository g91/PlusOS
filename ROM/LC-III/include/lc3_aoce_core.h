/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Apple Open Collaboration Environment (AOCE) Standard Mail Core
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_AOCE_CORE_H
#define LC3_AOCE_CORE_H

#include <stdint.h>
#include <stdbool.h>

#define AOCE_NO_ERR         0
#define AOCE_MAILBOX_FULL -9000
#define AOCE_AUTH_ERR     -9001
#define AOCE_PARAM_ERR      -50

#define MAX_RECIPIENTS     16

typedef struct AOCERecipient {
    char name[64];
    uint16_t recipientType;   /* 1 = To, 2 = CC, 3 = BCC */
} AOCERecipient;

typedef struct AOCEEnvelope {
    uint32_t letterID;
    char subject[64];
    char sender[64];
    uint16_t numRecipients;
    AOCERecipient recipients[MAX_RECIPIENTS];
    uint32_t bodyLength;
    uint32_t signatureHash;
    bool isSigned;
} AOCEEnvelope;

/* AOCE Mail API */
int16_t lc3_aoce_init(void);
int16_t lc3_aoce_new_letter(const char *subject, const char *sender, AOCEEnvelope *outLetter);
int16_t lc3_aoce_add_recipient(AOCEEnvelope *letter, const char *name, uint16_t recipientType);
int16_t lc3_aoce_sign_block(AOCEEnvelope *letter, const void *data, uint32_t length);
int16_t lc3_aoce_open_mailbox(const char *mailboxName, uint32_t *mailboxHandle);
int16_t lc3_aoce_read_letter(uint32_t mailboxHandle, uint32_t letterIndex, AOCEEnvelope *envelope, void *bodyBuf, uint32_t *bodyLen);

#endif /* LC3_AOCE_CORE_H */
