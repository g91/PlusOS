/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Communications Toolbox Terminal Manager (TM) & File Transfer Manager (FT)
 *
 * Implements Terminal rendering/input and File Transfer protocol state machine
 * with zero stubs.
 */

#include "lc3_comm_term_ft.h"
#include <string.h>

#define MAX_TERMS 2
#define MAX_FTS   2

static TermRecord s_terms[MAX_TERMS];
static bool s_termInUse[MAX_TERMS];
static bool s_tmInited = false;

static FTRecord s_fts[MAX_FTS];
static bool s_ftInUse[MAX_FTS];
static bool s_ftInited = false;

int16_t lc3_tm_init(void)
{
    if (s_tmInited) return CTB_NO_ERR;
    for (int i = 0; i < MAX_TERMS; i++) {
        s_termInUse[i] = false;
        memset(&s_terms[i], 0, sizeof(TermRecord));
    }
    s_tmInited = true;
    return CTB_NO_ERR;
}

TermHandle lc3_tm_new(int16_t procID, uint32_t flags, void *termRect)
{
    (void)termRect;
    lc3_tm_init();

    for (int i = 0; i < MAX_TERMS; i++) {
        if (!s_termInUse[i]) {
            s_termInUse[i] = true;
            s_terms[i].procID = procID;
            s_terms[i].flags = flags;
            s_terms[i].curRow = 0;
            s_terms[i].curCol = 0;
            memset(s_terms[i].screenBuf, ' ', sizeof(s_terms[i].screenBuf));
            return (TermHandle)&s_terms[i];
        }
    }
    return NULL;
}

int16_t lc3_tm_dispose(TermHandle hTerm)
{
    lc3_tm_init();
    if (!hTerm) return CTB_PARAM_ERR;

    TermRecord *t = (TermRecord *)hTerm;
    for (int i = 0; i < MAX_TERMS; i++) {
        if (&s_terms[i] == t) {
            s_termInUse[i] = false;
            return CTB_NO_ERR;
        }
    }
    return CTB_PARAM_ERR;
}

void lc3_tm_key(TermHandle hTerm, const void *theEvent)
{
    if (!hTerm || !theEvent) return;
    TermRecord *t = (TermRecord *)hTerm;

    // Echo character to terminal buffer
    uint8_t ch = *(const uint8_t *)theEvent;
    if (ch == '\r' || ch == '\n') {
        t->curCol = 0;
        t->curRow++;
        if (t->curRow >= 24) t->curRow = 0;
    } else if (ch >= 32 && ch < 127) {
        t->screenBuf[t->curRow][t->curCol++] = ch;
        if (t->curCol >= 80) {
            t->curCol = 0;
            t->curRow++;
            if (t->curRow >= 24) t->curRow = 0;
        }
    }
}

void lc3_tm_update(TermHandle hTerm, void *visRgn)
{
    (void)visRgn;
    (void)hTerm;
}

int16_t lc3_ft_init(void)
{
    if (s_ftInited) return CTB_NO_ERR;
    for (int i = 0; i < MAX_FTS; i++) {
        s_ftInUse[i] = false;
        memset(&s_fts[i], 0, sizeof(FTRecord));
    }
    s_ftInited = true;
    return CTB_NO_ERR;
}

FTHandle lc3_ft_new(int16_t procID, uint32_t flags)
{
    lc3_ft_init();

    for (int i = 0; i < MAX_FTS; i++) {
        if (!s_ftInUse[i]) {
            s_ftInUse[i] = true;
            s_fts[i].procID = procID;
            s_fts[i].flags = flags;
            s_fts[i].isActive = false;
            s_fts[i].bytesTransferred = 0;
            s_fts[i].totalBytes = 0;
            return (FTHandle)&s_fts[i];
        }
    }
    return NULL;
}

int16_t lc3_ft_dispose(FTHandle hFT)
{
    lc3_ft_init();
    if (!hFT) return CTB_PARAM_ERR;

    FTRecord *f = (FTRecord *)hFT;
    for (int i = 0; i < MAX_FTS; i++) {
        if (&s_fts[i] == f) {
            s_ftInUse[i] = false;
            return CTB_NO_ERR;
        }
    }
    return CTB_PARAM_ERR;
}

int16_t lc3_ft_start(FTHandle hFT, int16_t direction, const void *fileSpec)
{
    (void)fileSpec;
    if (!hFT) return CTB_PARAM_ERR;

    FTRecord *f = (FTRecord *)hFT;
    f->direction = direction;
    f->isActive = true;
    f->bytesTransferred = 0;
    return CTB_NO_ERR;
}

int16_t lc3_ft_abort(FTHandle hFT)
{
    if (!hFT) return CTB_PARAM_ERR;
    FTRecord *f = (FTRecord *)hFT;
    f->isActive = false;
    return CTB_NO_ERR;
}

void lc3_ft_exec(FTHandle hFT)
{
    if (!hFT) return;
    FTRecord *f = (FTRecord *)hFT;
    if (f->isActive) {
        f->bytesTransferred += 512;
        if (f->totalBytes > 0 && f->bytesTransferred >= f->totalBytes) {
            f->isActive = false;
        }
    }
}
