/* lc3_cqd_engine.h - Macintosh LC III Color QuickDraw Advanced Pipeline & Text Engine
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x028758 - 0x02A914
 */
#ifndef LC3_CQD_ENGINE_H
#define LC3_CQD_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

/* Font Style Bits */
#define FONT_STYLE_BOLD      0x01
#define FONT_STYLE_ITALIC    0x02
#define FONT_STYLE_UNDERLINE 0x04
#define FONT_STYLE_OUTLINE   0x08
#define FONT_STYLE_SHADOW    0x10
#define FONT_STYLE_CONDENSE  0x20
#define FONT_STYLE_EXTEND    0x40

/* Font Info Record */
typedef struct {
    int16_t ascent;
    int16_t descent;
    int16_t widMax;
    int16_t leading;
} LC3FontInfo;

/* QuickDraw Pen State */
typedef struct {
    int16_t pnLocH;
    int16_t pnLocV;
    int16_t pnSizeH;
    int16_t pnSizeV;
    int16_t pnMode;
    uint8_t pnPat[8];
} LC3PenState;

/* Line Drawing Coordinates */
typedef struct {
    int16_t x0;
    int16_t y0;
    int16_t x1;
    int16_t y1;
} LC3LineCoords;

/* CQD Engine API */
void    lc3_cqd_moveto(int16_t h, int16_t v);
void    lc3_cqd_move(int16_t dh, int16_t dv);
void    lc3_cqd_lineto(int16_t h, int16_t v);
void    lc3_cqd_line(int16_t dh, int16_t dv);
void    lc3_cqd_putline(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void    lc3_cqd_putline_slanted(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void    lc3_cqd_textface(uint8_t face);
int16_t lc3_cqd_charwidth(char c);
int16_t lc3_cqd_stringwidth(const char *str);
void    lc3_cqd_getfontinfo(LC3FontInfo *info);
void    lc3_cqd_draw_char_styled(char c, int16_t x, int16_t y, uint8_t style);
void    lc3_cqd_draw_string(const char *str, int16_t x, int16_t y);
void    lc3_cqd_measure_text(const char *text, uint16_t count, int16_t *charLocs);

#endif /* LC3_CQD_ENGINE_H */
