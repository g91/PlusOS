/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Multi-Style TextEdit Engine (System 7 32-bit Extensions)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_TEXTEDIT_H
#define LC3_TEXTEDIT_H

#include <stdint.h>
#include <stdbool.h>

#define TE_JUST_LEFT     0
#define TE_JUST_CENTER   1
#define TE_JUST_RIGHT   -1
#define TE_FORCE_LEFT   -2

#define TE_DO_FONT       1
#define TE_DO_FACE       2
#define TE_DO_SIZE       4
#define TE_DO_COLOR      8
#define TE_DO_ALL       15

#define TE_FIND          0
#define TE_HILITE        1
#define TE_DRAW          2
#define TE_CARET         3

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} TERect;

typedef struct {
    int16_t v;
    int16_t h;
} TEPoint;

typedef struct {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} TERGBColor;

typedef struct {
    int16_t stCount;
    int16_t stHeight;
    int16_t stAscent;
    int16_t stFont;
    uint8_t stFace;
    uint8_t filler;
    int16_t stSize;
    TERGBColor stColor;
} STElement;

typedef struct {
    int32_t startChar;
    int16_t styleIndex;
} StyleRun;

typedef struct {
    int16_t nRuns;
    int16_t nStyles;
    STElement styles[32];
    StyleRun runs[64];
    int16_t nullStyleIndex;
} TEStyleRec;

typedef struct {
    TERect destRect;
    TERect viewRect;
    TERect selRect;
    int16_t lineHeight;
    int16_t fontAscent;
    TEPoint selPoint;
    int16_t selStart;
    int16_t selEnd;
    int16_t active;
    int16_t wordBreak;
    int16_t clikLoop;
    int32_t clickTime;
    int16_t clickLoc;
    int16_t caretTime;
    int16_t caretState;
    int16_t just;
    int16_t teLength;
    char text[4096];
    int16_t txFont;
    uint8_t txFace;
    uint8_t txMode;
    int16_t txSize;
    int16_t nLines;
    int16_t lineStarts[256];
    bool isStyled;
    TEStyleRec styleRec;
} TERecord, *TEPtr, **TEHandle;

/* Monostyle and Multi-Style TextEdit Functions */
TEHandle lc3_te_new(const TERect *destRect, const TERect *viewRect);
TEHandle lc3_te_style_new(const TERect *destRect, const TERect *viewRect);
void lc3_te_dispose(TEHandle hTE);
void lc3_te_set_text(const char *text, int32_t length, TEHandle hTE);
int32_t lc3_te_get_text(TEHandle hTE, char *outBuffer, int32_t maxLen);
void lc3_te_idle(TEHandle hTE);
void lc3_te_click(TEPoint pt, bool extend, TEHandle hTE);
void lc3_te_set_select(int32_t selStart, int32_t selEnd, TEHandle hTE);
void lc3_te_activate(TEHandle hTE);
void lc3_te_deactivate(TEHandle hTE);
void lc3_te_key(char key, TEHandle hTE);
void lc3_te_cut(TEHandle hTE);
void lc3_te_copy(TEHandle hTE);
void lc3_te_paste(TEHandle hTE);
void lc3_te_delete(TEHandle hTE);
void lc3_te_insert(const char *text, int32_t length, TEHandle hTE);
void lc3_te_set_just(int16_t just, TEHandle hTE);
void lc3_te_update(const TERect *rUpdate, TEHandle hTE);
void lc3_te_textbox(const char *text, int32_t length, const TERect *box, int16_t just);
void lc3_te_scroll(int16_t dh, int16_t dv, TEHandle hTE);
void lc3_te_sel_view(TEHandle hTE);
void lc3_te_pin_scroll(int16_t dh, int16_t dv, TEHandle hTE);
void lc3_te_auto_view(bool autoView, TEHandle hTE);

/* Multi-Style Traps */
void lc3_te_get_style(int32_t offset, STElement *element, int16_t *lineHeight, int16_t *fontAscent, TEHandle hTE);
void lc3_te_set_style(int16_t mode, const STElement *newStyle, bool redraw, TEHandle hTE);
void lc3_te_styl_paste(TEHandle hTE);
void lc3_te_replace_style(int16_t mode, const STElement *oldStyle, const STElement *newStyle, bool redraw, TEHandle hTE);
int16_t lc3_te_num_styles(int32_t selStart, int32_t selEnd, TEHandle hTE);
bool lc3_te_continuous_style(int16_t *mode, STElement *element, TEHandle hTE);
void lc3_te_use_style_scrap(int32_t rangeStart, int32_t rangeEnd, TEHandle hTE);
void lc3_te_styl_insert(const char *text, int32_t length, const STElement *style, TEHandle hTE);

#endif /* LC3_TEXTEDIT_H */
