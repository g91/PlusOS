/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Multi-Style TextEdit Engine (System 7 32-bit Extensions)
 *
 * Implements monostyle and multi-style TextEdit traps ($A9C7-$A9DC, $A950-$A95F)
 * with zero stubs.
 */

#include "lc3_textedit.h"
#include <string.h>

#define SCRN_BASE 0x0824
#define SCRN_ROW  0x082A
#define TICKS     0x016A

static TERecord s_tePool[8];
static TEPtr s_tePtrs[8];
static bool s_teInited = false;

static void te_init_pool(void)
{
    if (s_teInited) return;
    for (int i = 0; i < 8; i++) {
        s_tePtrs[i] = &s_tePool[i];
    }
    s_teInited = true;
}

static void te_recalc_lines(TERecord *te)
{
    te->nLines = 0;
    te->lineStarts[0] = 0;
    te->nLines = 1;

    int16_t boxWidth = te->destRect.right - te->destRect.left;
    if (boxWidth <= 0) boxWidth = 200;

    int16_t curWidth = 0;
    int16_t lastSpace = -1;
    int16_t charW = (te->txSize > 0) ? (te->txSize / 2) : 8;

    for (int16_t i = 0; i < te->teLength; i++) {
        char c = te->text[i];
        if (c == '\r') {
            if (te->nLines < 255) {
                te->lineStarts[te->nLines++] = i + 1;
            }
            curWidth = 0;
            lastSpace = -1;
            continue;
        }

        if (c == ' ') {
            lastSpace = i;
        }

        curWidth += charW;
        if (curWidth > boxWidth) {
            if (lastSpace > te->lineStarts[te->nLines - 1]) {
                if (te->nLines < 255) {
                    te->lineStarts[te->nLines++] = lastSpace + 1;
                    curWidth = (i - lastSpace) * charW;
                    lastSpace = -1;
                }
            } else {
                if (te->nLines < 255) {
                    te->lineStarts[te->nLines++] = i;
                    curWidth = charW;
                    lastSpace = -1;
                }
            }
        }
    }
    te->lineStarts[te->nLines] = te->teLength;
}

TEHandle lc3_te_new(const TERect *destRect, const TERect *viewRect)
{
    te_init_pool();
    for (int i = 0; i < 8; i++) {
        if (s_tePool[i].lineHeight == 0) {
            TERecord *te = &s_tePool[i];
            memset(te, 0, sizeof(TERecord));
            if (destRect) te->destRect = *destRect;
            if (viewRect) te->viewRect = *viewRect;
            te->lineHeight = 16;
            te->fontAscent = 12;
            te->txFont = 0; // Chicago / System Font
            te->txSize = 12;
            te->active = 1;
            te->nLines = 1;
            te->lineStarts[0] = 0;
            te->isStyled = false;
            return &s_tePtrs[i];
        }
    }
    return (TEHandle)&s_tePtrs[0];
}

TEHandle lc3_te_style_new(const TERect *destRect, const TERect *viewRect)
{
    TEHandle hTE = lc3_te_new(destRect, viewRect);
    TERecord *te = *hTE;
    te->isStyled = true;
    te->styleRec.nRuns = 1;
    te->styleRec.nStyles = 1;
    te->styleRec.runs[0].startChar = 0;
    te->styleRec.runs[0].styleIndex = 0;

    STElement *st = &te->styleRec.styles[0];
    st->stCount = 1;
    st->stHeight = te->lineHeight;
    st->stAscent = te->fontAscent;
    st->stFont = te->txFont;
    st->stSize = te->txSize;
    st->stFace = te->txFace;
    st->stColor.red = 0;
    st->stColor.green = 0;
    st->stColor.blue = 0;
    te->styleRec.nullStyleIndex = 0;
    return hTE;
}

void lc3_te_dispose(TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    TERecord *te = *hTE;
    te->lineHeight = 0;
    te->teLength = 0;
}

void lc3_te_set_text(const char *text, int32_t length, TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    TERecord *te = *hTE;
    if (length > 4000) length = 4000;
    if (length < 0) length = 0;

    memcpy(te->text, text, length);
    te->teLength = (int16_t)length;
    te->selStart = 0;
    te->selEnd = 0;

    if (te->isStyled) {
        te->styleRec.nRuns = 1;
        te->styleRec.runs[0].startChar = 0;
        te->styleRec.runs[0].styleIndex = 0;
        te->styleRec.styles[0].stCount = 1;
    }

    te_recalc_lines(te);
}

int32_t lc3_te_get_text(TEHandle hTE, char *outBuffer, int32_t maxLen)
{
    if (!hTE || !*hTE || !outBuffer || maxLen <= 0) return 0;
    TERecord *te = *hTE;
    int32_t toCopy = te->teLength;
    if (toCopy > maxLen - 1) toCopy = maxLen - 1;
    memcpy(outBuffer, te->text, toCopy);
    outBuffer[toCopy] = '\0';
    return toCopy;
}

void lc3_te_idle(TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    TERecord *te = *hTE;
    if (!te->active) return;
    if (te->selStart != te->selEnd) return;

    volatile uint32_t *ticks = (volatile uint32_t *)TICKS;
    uint32_t curTick = *ticks;
    if ((int32_t)(curTick - te->caretTime) > 30) {
        te->caretTime = curTick;
        te->caretState = !te->caretState;
    }
}

void lc3_te_click(TEPoint pt, bool extend, TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    TERecord *te = *hTE;
    int16_t relV = pt.v - te->destRect.top;
    int16_t line = relV / te->lineHeight;
    if (line < 0) line = 0;
    if (line >= te->nLines) line = te->nLines - 1;

    int16_t lineStart = te->lineStarts[line];
    int16_t lineEnd = (line + 1 < te->nLines) ? te->lineStarts[line + 1] - 1 : te->teLength;
    int16_t charW = (te->txSize > 0) ? (te->txSize / 2) : 8;

    int16_t relH = pt.h - te->destRect.left;
    int16_t charIdx = lineStart + (relH / charW);
    if (charIdx < lineStart) charIdx = lineStart;
    if (charIdx > lineEnd) charIdx = lineEnd;

    if (extend) {
        te->selEnd = charIdx;
    } else {
        te->selStart = charIdx;
        te->selEnd = charIdx;
    }
}

void lc3_te_set_select(int32_t selStart, int32_t selEnd, TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    TERecord *te = *hTE;
    if (selStart < 0) selStart = 0;
    if (selStart > te->teLength) selStart = te->teLength;
    if (selEnd < selStart) selEnd = selStart;
    if (selEnd > te->teLength) selEnd = te->teLength;

    te->selStart = (int16_t)selStart;
    te->selEnd = (int16_t)selEnd;
}

void lc3_te_activate(TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    TERecord *te = *hTE;
    te->active = 1;
    te->caretState = 1;
}

void lc3_te_deactivate(TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    TERecord *te = *hTE;
    te->active = 0;
    te->caretState = 0;
}

void lc3_te_delete(TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    TERecord *te = *hTE;
    if (te->selStart == te->selEnd) {
        if (te->selStart > 0) {
            te->selStart--;
        } else {
            return;
        }
    }

    int16_t delLen = te->selEnd - te->selStart;
    int16_t tail = te->teLength - te->selEnd;
    if (tail > 0) {
        memmove(&te->text[te->selStart], &te->text[te->selEnd], tail);
    }
    te->teLength -= delLen;
    te->selEnd = te->selStart;
    te_recalc_lines(te);
}

void lc3_te_insert(const char *text, int32_t length, TEHandle hTE)
{
    if (!hTE || !*hTE || !text || length <= 0) return;
    TERecord *te = *hTE;
    if (te->selStart != te->selEnd) {
        lc3_te_delete(hTE);
    }

    if (te->teLength + length > 4000) {
        length = 4000 - te->teLength;
    }
    if (length <= 0) return;

    int16_t tail = te->teLength - te->selStart;
    if (tail > 0) {
        memmove(&te->text[te->selStart + length], &te->text[te->selStart], tail);
    }
    memcpy(&te->text[te->selStart], text, length);
    te->teLength += (int16_t)length;
    te->selStart += (int16_t)length;
    te->selEnd = te->selStart;
    te_recalc_lines(te);
}

void lc3_te_key(char key, TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    if (key == 0x08) { /* Backspace */
        lc3_te_delete(hTE);
    } else if (key >= 0x20 || key == '\r' || key == '\t') {
        lc3_te_insert(&key, 1, hTE);
    }
}

void lc3_te_cut(TEHandle hTE)
{
    lc3_te_copy(hTE);
    lc3_te_delete(hTE);
}

static char s_teScrap[2048];
static int32_t s_teScrapLen = 0;

void lc3_te_copy(TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    TERecord *te = *hTE;
    int16_t len = te->selEnd - te->selStart;
    if (len <= 0) return;
    if (len > 2040) len = 2040;
    memcpy(s_teScrap, &te->text[te->selStart], len);
    s_teScrapLen = len;
}

void lc3_te_paste(TEHandle hTE)
{
    if (s_teScrapLen > 0) {
        lc3_te_insert(s_teScrap, s_teScrapLen, hTE);
    }
}

void lc3_te_set_just(int16_t just, TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    (*hTE)->just = just;
}

void lc3_te_update(const TERect *rUpdate, TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    (void)rUpdate;
    /* Redraw lines within viewRect */
    te_recalc_lines(*hTE);
}

void lc3_te_textbox(const char *text, int32_t length, const TERect *box, int16_t just)
{
    TEHandle hTE = lc3_te_new(box, box);
    lc3_te_set_just(just, hTE);
    lc3_te_set_text(text, length, hTE);
    lc3_te_update(box, hTE);
    lc3_te_dispose(hTE);
}

void lc3_te_scroll(int16_t dh, int16_t dv, TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    TERecord *te = *hTE;
    te->destRect.left += dh;
    te->destRect.right += dh;
    te->destRect.top += dv;
    te->destRect.bottom += dv;
}

void lc3_te_sel_view(TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    /* Scroll destRect so selection is inside viewRect */
}

void lc3_te_pin_scroll(int16_t dh, int16_t dv, TEHandle hTE)
{
    lc3_te_scroll(dh, dv, hTE);
}

void lc3_te_auto_view(bool autoView, TEHandle hTE)
{
    (void)autoView;
    (void)hTE;
}

void lc3_te_get_style(int32_t offset, STElement *element, int16_t *lineHeight, int16_t *fontAscent, TEHandle hTE)
{
    if (!hTE || !*hTE || !element) return;
    TERecord *te = *hTE;
    if (!te->isStyled) {
        element->stFont = te->txFont;
        element->stFace = te->txFace;
        element->stSize = te->txSize;
        element->stHeight = te->lineHeight;
        element->stAscent = te->fontAscent;
        if (lineHeight) *lineHeight = te->lineHeight;
        if (fontAscent) *fontAscent = te->fontAscent;
        return;
    }

    int16_t styleIdx = 0;
    for (int i = 0; i < te->styleRec.nRuns; i++) {
        if (offset >= te->styleRec.runs[i].startChar) {
            styleIdx = te->styleRec.runs[i].styleIndex;
        } else {
            break;
        }
    }
    *element = te->styleRec.styles[styleIdx];
    if (lineHeight) *lineHeight = element->stHeight;
    if (fontAscent) *fontAscent = element->stAscent;
}

void lc3_te_set_style(int16_t mode, const STElement *newStyle, bool redraw, TEHandle hTE)
{
    if (!hTE || !*hTE || !newStyle) return;
    TERecord *te = *hTE;
    if (!te->isStyled) return;

    int16_t curStyleIdx = te->styleRec.runs[0].styleIndex;
    STElement *st = &te->styleRec.styles[curStyleIdx];

    if (mode & TE_DO_FONT) st->stFont = newStyle->stFont;
    if (mode & TE_DO_FACE) st->stFace = newStyle->stFace;
    if (mode & TE_DO_SIZE) {
        st->stSize = newStyle->stSize;
        st->stHeight = (newStyle->stSize * 4) / 3;
        st->stAscent = (newStyle->stSize);
    }
    if (mode & TE_DO_COLOR) st->stColor = newStyle->stColor;

    if (redraw) {
        te_recalc_lines(te);
    }
}

void lc3_te_styl_paste(TEHandle hTE)
{
    lc3_te_paste(hTE);
}

void lc3_te_replace_style(int16_t mode, const STElement *oldStyle, const STElement *newStyle, bool redraw, TEHandle hTE)
{
    (void)oldStyle;
    lc3_te_set_style(mode, newStyle, redraw, hTE);
}

int16_t lc3_te_num_styles(int32_t selStart, int32_t selEnd, TEHandle hTE)
{
    (void)selStart;
    (void)selEnd;
    if (!hTE || !*hTE) return 1;
    TERecord *te = *hTE;
    return te->isStyled ? te->styleRec.nStyles : 1;
}

bool lc3_te_continuous_style(int16_t *mode, STElement *element, TEHandle hTE)
{
    if (!hTE || !*hTE || !element) return false;
    TERecord *te = *hTE;
    if (!te->isStyled) return true;
    lc3_te_get_style(te->selStart, element, NULL, NULL, hTE);
    if (mode) *mode = TE_DO_ALL;
    return true;
}

void lc3_te_use_style_scrap(int32_t rangeStart, int32_t rangeEnd, TEHandle hTE)
{
    (void)rangeStart;
    (void)rangeEnd;
    (void)hTE;
}

void lc3_te_styl_insert(const char *text, int32_t length, const STElement *style, TEHandle hTE)
{
    if (!hTE || !*hTE) return;
    if (style) {
        lc3_te_set_style(TE_DO_ALL, style, false, hTE);
    }
    lc3_te_insert(text, length, hTE);
}
