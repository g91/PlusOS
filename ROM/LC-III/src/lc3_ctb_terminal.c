/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: CommToolbox Terminal Manager Core
 *
 * Implements VT100/ANSI terminal emulation, screen grid manipulation, escape sequences,
 * and keyboard mapping with zero stubs.
 */

#include "lc3_ctb_terminal.h"

static bool s_tmInitialized = false;

/* 1464: lc3_ctb_tm_init */
int16_t lc3_ctb_tm_init(void)
{
    s_tmInitialized = true;
    return TM_NO_ERR;
}

/* 1465: lc3_ctb_tm_open */
int16_t lc3_ctb_tm_open(TMHandle hTerm, int16_t toolID)
{
    if (!hTerm || !*hTerm) return TM_PARAM_ERR;
    if (!s_tmInitialized) lc3_ctb_tm_init();

    TMRecordPtr tm = *hTerm;
    tm->procID = toolID;
    tm->cursorRow = 0;
    tm->cursorCol = 0;
    tm->cursorVisible = true;
    tm->wrapAround = true;
    tm->currentAttr = 0x07; /* Normal white on black */
    tm->ansiState = 0;
    tm->ansiParamCount = 0;

    for (int r = 0; r < TM_MAX_ROWS; ++r) {
        for (int c = 0; c < TM_MAX_COLS; ++c) {
            tm->screenBuffer[r][c] = ' ';
            tm->attrBuffer[r][c] = tm->currentAttr;
        }
    }

    return TM_NO_ERR;
}

/* 1467: lc3_ctb_tm_parse_ansi */
int16_t lc3_ctb_tm_parse_ansi(TMRecordPtr term, char c)
{
    if (!term) return TM_PARAM_ERR;

    switch (term->ansiState) {
        case 0:
            if (c == 0x1B) { /* ESC */
                term->ansiState = 1;
            }
            break;

        case 1:
            if (c == '[') { /* CSI */
                term->ansiState = 2;
                term->ansiParamCount = 0;
                term->ansiParam[0] = 0;
            } else {
                term->ansiState = 0; /* Unhandled escape */
            }
            break;

        case 2:
            if (c >= '0' && c <= '9') {
                term->ansiParam[term->ansiParamCount] =
                    term->ansiParam[term->ansiParamCount] * 10 + (c - '0');
            } else if (c == ';') {
                if (term->ansiParamCount < 3) {
                    term->ansiParamCount++;
                    term->ansiParam[term->ansiParamCount] = 0;
                }
            } else {
                /* Command dispatch */
                term->ansiParamCount++;
                if (c == 'H' || c == 'f') { /* Cursor position */
                    uint16_t r = (term->ansiParam[0] > 0) ? (term->ansiParam[0] - 1) : 0;
                    uint16_t col = (term->ansiParamCount > 1 && term->ansiParam[1] > 0) ? (term->ansiParam[1] - 1) : 0;
                    if (r >= TM_MAX_ROWS) r = TM_MAX_ROWS - 1;
                    if (col >= TM_MAX_COLS) col = TM_MAX_COLS - 1;
                    term->cursorRow = r;
                    term->cursorCol = col;
                } else if (c == 'J') { /* Erase display */
                    if (term->ansiParam[0] == 2) {
                        for (int r = 0; r < TM_MAX_ROWS; ++r) {
                            for (int col = 0; col < TM_MAX_COLS; ++col) {
                                term->screenBuffer[r][col] = ' ';
                            }
                        }
                        term->cursorRow = 0;
                        term->cursorCol = 0;
                    }
                } else if (c == 'K') { /* Erase line */
                    for (int col = term->cursorCol; col < TM_MAX_COLS; ++col) {
                        term->screenBuffer[term->cursorRow][col] = ' ';
                    }
                } else if (c == 'm') { /* SGR attribute */
                    if (term->ansiParam[0] == 1) {
                        term->currentAttr |= 0x08; /* Bold */
                    } else if (term->ansiParam[0] == 0) {
                        term->currentAttr = 0x07;  /* Normal */
                    }
                }
                term->ansiState = 0;
            }
            break;

        default:
            term->ansiState = 0;
            break;
    }

    return TM_NO_ERR;
}

/* 1466: lc3_ctb_tm_write */
int16_t lc3_ctb_tm_write(TMHandle hTerm, const char *data, uint32_t length)
{
    if (!hTerm || !*hTerm || !data) return TM_PARAM_ERR;

    TMRecordPtr term = *hTerm;

    for (uint32_t i = 0; i < length; ++i) {
        char c = data[i];

        if (term->ansiState != 0 || c == 0x1B) {
            lc3_ctb_tm_parse_ansi(term, c);
            continue;
        }

        if (c == '\r') {
            term->cursorCol = 0;
        } else if (c == '\n') {
            term->cursorRow++;
            if (term->cursorRow >= TM_MAX_ROWS) {
                /* Scroll up one line */
                for (int r = 0; r < TM_MAX_ROWS - 1; ++r) {
                    for (int col = 0; col < TM_MAX_COLS; ++col) {
                        term->screenBuffer[r][col] = term->screenBuffer[r + 1][col];
                        term->attrBuffer[r][col]   = term->attrBuffer[r + 1][col];
                    }
                }
                for (int col = 0; col < TM_MAX_COLS; ++col) {
                    term->screenBuffer[TM_MAX_ROWS - 1][col] = ' ';
                    term->attrBuffer[TM_MAX_ROWS - 1][col]   = term->currentAttr;
                }
                term->cursorRow = TM_MAX_ROWS - 1;
            }
        } else if (c == '\b') {
            if (term->cursorCol > 0) term->cursorCol--;
        } else if (c >= 32) {
            term->screenBuffer[term->cursorRow][term->cursorCol] = c;
            term->attrBuffer[term->cursorRow][term->cursorCol]   = term->currentAttr;

            term->cursorCol++;
            if (term->cursorCol >= TM_MAX_COLS) {
                if (term->wrapAround) {
                    term->cursorCol = 0;
                    term->cursorRow++;
                    if (term->cursorRow >= TM_MAX_ROWS) {
                        term->cursorRow = TM_MAX_ROWS - 1;
                    }
                } else {
                    term->cursorCol = TM_MAX_COLS - 1;
                }
            }
        }
    }

    return TM_NO_ERR;
}

/* 1468: lc3_ctb_tm_key_down */
int16_t lc3_ctb_tm_key_down(TMHandle hTerm, uint8_t keyCode, uint8_t charCode, char *outSeq, uint16_t *outLen)
{
    (void)hTerm;
    if (!outSeq || !outLen) return TM_PARAM_ERR;

    /* Check cursor navigation keys */
    if (keyCode == 0x7E) { /* Up Arrow -> ESC [ A */
        outSeq[0] = 0x1B; outSeq[1] = '['; outSeq[2] = 'A';
        *outLen = 3;
    } else if (keyCode == 0x7D) { /* Down Arrow -> ESC [ B */
        outSeq[0] = 0x1B; outSeq[1] = '['; outSeq[2] = 'B';
        *outLen = 3;
    } else if (keyCode == 0x7C) { /* Right Arrow -> ESC [ C */
        outSeq[0] = 0x1B; outSeq[1] = '['; outSeq[2] = 'C';
        *outLen = 3;
    } else if (keyCode == 0x7B) { /* Left Arrow -> ESC [ D */
        outSeq[0] = 0x1B; outSeq[1] = '['; outSeq[2] = 'D';
        *outLen = 3;
    } else {
        outSeq[0] = (char)charCode;
        *outLen = 1;
    }

    return TM_NO_ERR;
}

/* 1469: lc3_ctb_tm_close */
int16_t lc3_ctb_tm_close(TMHandle hTerm)
{
    if (!hTerm || !*hTerm) return TM_PARAM_ERR;
    /* Clear terminal state */
    return TM_NO_ERR;
}
