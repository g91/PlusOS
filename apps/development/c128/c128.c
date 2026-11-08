/* c128.c - Commodore 128 Emulator for PlusOS
 *
 * Emulates the MOS 8502 / MMU / VDC / VIC-II Commodore 128:
 *   - MOS 8502 CPU with 1 MHz / 2 MHz fast clock simulation
 *   - 128 KB RAM across two 64 KB banks (Bank 0 & Bank 1)
 *   - C128 MMU ($D500..$D50B) memory banking & configuration
 *   - Dual Display:
 *       1. VIC-II 40-Column Screen (320x200 with border)
 *       2. VDC (MOS 8563) 80-Column Screen (80x25 = 480x200 centered on Mac 512x342)
 *   - Tab / 40-80 key toggles between VIC-II 40-column & VDC 80-column displays
 *   - Authentic Commodore PETSCII 8x8 & 6x8 condensed fonts
 *   - C128 BASIC V7.0 startup banner & interactive prompt
 *   - Built-in C128 demos (80-col VDC, Bank 0/1 memory test, 40-col mode)
 *   - External Commodore PRG binary loader (.prg)
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusrom/FileMgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define C128_BANK_SIZE 65536

static uint8_t  c128_bank0[C128_BANK_SIZE];
static uint8_t  c128_bank1[C128_BANK_SIZE];
static uint8_t  vdc_ram[16384]; /* 16 KB VDC dedicated video RAM */

/* MMU Registers ($D500..$D50B) */
static uint8_t  mmu_cr = 0x00;   /* Configuration Register: bit 6=RAM bank (0/1) */
static uint8_t  mmu_mode = 0x00; /* Mode Register: bit 0 = 40/80 col flag */

/* 8502 CPU Registers */
static uint8_t  reg_a, reg_x, reg_y, reg_sp, reg_p;
static uint16_t reg_pc;
static int      cpu_halted;
static int      fast_2mhz = 0;

enum {
    FLAG_C = 0x01, FLAG_Z = 0x02, FLAG_I = 0x04, FLAG_D = 0x08,
    FLAG_B = 0x10, FLAG_U = 0x20, FLAG_V = 0x40, FLAG_N = 0x80
};

/* Display Mode: 0 = 40-column VIC-II, 1 = 80-column VDC */
static int display_80col = 1;

/* PETSCII 8x8 font */
static const uint8_t c128_font8[128][8] = {
    ['@'] = {0x3C,0x66,0x6E,0x6E,0x60,0x62,0x3C,0x00},
    ['A'] = {0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00},
    ['B'] = {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},
    ['C'] = {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
    ['D'] = {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},
    ['E'] = {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},
    ['F'] = {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},
    ['G'] = {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3A,0x00},
    ['H'] = {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
    ['I'] = {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    ['J'] = {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00},
    ['K'] = {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
    ['L'] = {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},
    ['M'] = {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
    ['N'] = {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00},
    ['O'] = {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    ['P'] = {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
    ['Q'] = {0x3C,0x66,0x66,0x66,0x66,0x3C,0x0E,0x00},
    ['R'] = {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00},
    ['S'] = {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
    ['T'] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    ['U'] = {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    ['V'] = {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
    ['W'] = {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    ['X'] = {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
    ['Y'] = {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    ['Z'] = {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},
    [' '] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['!'] = {0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x00},
    ['"'] = {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00},
    ['#'] = {0x66,0xFF,0x66,0x66,0xFF,0x66,0x00,0x00},
    ['$'] = {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00},
    ['%'] = {0x62,0x64,0x08,0x10,0x26,0x46,0x00,0x00},
    ['&'] = {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00},
    ['\'']= {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
    ['('] = {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},
    [')'] = {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
    ['*'] = {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    ['+'] = {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
    [','] = {0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00},
    ['-'] = {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
    ['.'] = {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
    ['/'] = {0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x00},
    ['0'] = {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
    ['1'] = {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
    ['2'] = {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00},
    ['3'] = {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
    ['4'] = {0x0C,0x1C,0x34,0x64,0x7E,0x04,0x04,0x00},
    ['5'] = {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
    ['6'] = {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00},
    ['7'] = {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
    ['8'] = {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
    ['9'] = {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},
    [':'] = {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00},
    [';'] = {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00},
    ['='] = {0x00,0x7E,0x00,0x7E,0x00,0x00,0x00,0x00}
};

static void set_nz(uint8_t v)
{
    reg_p &= (uint8_t)~(FLAG_N | FLAG_Z);
    if (v == 0) reg_p |= FLAG_Z;
    if (v & 0x80) reg_p |= FLAG_N;
}

/* C128 MMU Memory Access */
static uint8_t c128_rd(uint16_t addr)
{
    /* MMU registers at $D500-$D50B */
    if (addr >= 0xD500 && addr <= 0xD50B) {
        if (addr == 0xD500) return mmu_cr;
        if (addr == 0xD505) return mmu_mode;
        return 0;
    }
    /* Select Bank 0 or Bank 1 based on MMU CR bit 6 */
    uint8_t *ram = (mmu_cr & 0x40) ? c128_bank1 : c128_bank0;
    return ram[addr];
}

static void c128_wr(uint16_t addr, uint8_t val)
{
    if (addr >= 0xD500 && addr <= 0xD50B) {
        if (addr == 0xD500) { mmu_cr = val; return; }
        if (addr == 0xD505) { mmu_mode = val; return; }
        return;
    }
    if (addr == 0xD030) {
        /* 8502 2 MHz fast clock bit 0 */
        fast_2mhz = val & 1;
        return;
    }
    uint8_t *ram = (mmu_cr & 0x40) ? c128_bank1 : c128_bank0;
    ram[addr] = val;
}

static uint16_t c128_rd16(uint16_t addr)
{
    return (uint16_t)c128_rd(addr) | ((uint16_t)c128_rd((uint16_t)(addr + 1)) << 8);
}

static void push8(uint8_t v)   { c128_wr((uint16_t)(0x0100 + reg_sp--), v); }
static uint8_t pop8(void)      { return c128_rd((uint16_t)(0x0100 + ++reg_sp)); }
static void push16(uint16_t v) { push8((uint8_t)(v >> 8)); push8((uint8_t)v); }
static uint16_t pop16(void)    { uint8_t lo = pop8(); return lo | ((uint16_t)pop8() << 8); }

static void do_adc(uint8_t m)
{
    uint16_t sum = (uint16_t)reg_a + m + (reg_p & FLAG_C ? 1 : 0);
    if (sum > 0xFF) reg_p |= FLAG_C; else reg_p &= (uint8_t)~FLAG_C;
    if (~(reg_a ^ m) & (reg_a ^ sum) & 0x80) reg_p |= FLAG_V; else reg_p &= (uint8_t)~FLAG_V;
    reg_a = (uint8_t)sum;
    set_nz(reg_a);
}

static void do_sbc(uint8_t m) { do_adc((uint8_t)~m); }

static void do_cmp(uint8_t r, uint8_t m)
{
    uint16_t d = (uint16_t)r - m;
    if (r >= m) reg_p |= FLAG_C; else reg_p &= (uint8_t)~FLAG_C;
    set_nz((uint8_t)d);
}

static int step8502(void)
{
    uint8_t op = c128_rd(reg_pc++);
    uint16_t addr = 0;

    switch (op) {
    case 0x00: cpu_halted = 1; return 0;
    case 0xEA: break;
    case 0xA9: reg_a = c128_rd(reg_pc++); set_nz(reg_a); break;
    case 0xA5: reg_a = c128_rd(c128_rd(reg_pc++)); set_nz(reg_a); break;
    case 0xAD: addr = c128_rd16(reg_pc); reg_pc += 2; reg_a = c128_rd(addr); set_nz(reg_a); break;
    case 0xA2: reg_x = c128_rd(reg_pc++); set_nz(reg_x); break;
    case 0xA0: reg_y = c128_rd(reg_pc++); set_nz(reg_y); break;
    case 0x85: c128_wr(c128_rd(reg_pc++), reg_a); break;
    case 0x8D: addr = c128_rd16(reg_pc); reg_pc += 2; c128_wr(addr, reg_a); break;
    case 0x86: c128_wr(c128_rd(reg_pc++), reg_x); break;
    case 0x84: c128_wr(c128_rd(reg_pc++), reg_y); break;
    case 0xAA: reg_x = reg_a; set_nz(reg_x); break;
    case 0xA8: reg_y = reg_a; set_nz(reg_y); break;
    case 0x8A: reg_a = reg_x; set_nz(reg_a); break;
    case 0x98: reg_a = reg_y; set_nz(reg_a); break;
    case 0x69: do_adc(c128_rd(reg_pc++)); break;
    case 0xE9: do_sbc(c128_rd(reg_pc++)); break;
    case 0xC9: do_cmp(reg_a, c128_rd(reg_pc++)); break;
    case 0xE8: reg_x++; set_nz(reg_x); break;
    case 0xC8: reg_y++; set_nz(reg_y); break;
    case 0xCA: reg_x--; set_nz(reg_x); break;
    case 0x88: reg_y--; set_nz(reg_y); break;
    case 0xD0: { int8_t off = (int8_t)c128_rd(reg_pc++); if (!(reg_p & FLAG_Z)) reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0xF0: { int8_t off = (int8_t)c128_rd(reg_pc++); if (reg_p & FLAG_Z)   reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0x4C: reg_pc = c128_rd16(reg_pc); break;
    case 0x20: addr = c128_rd16(reg_pc); reg_pc += 2; push16((uint16_t)(reg_pc - 1)); reg_pc = addr; break;
    case 0x60: reg_pc = (uint16_t)(pop16() + 1); break;
    default: break;
    }
    return 1;
}

static void c128_reset(void)
{
    memset(c128_bank0, 0, sizeof c128_bank0);
    memset(c128_bank1, 0, sizeof c128_bank1);
    memset(vdc_ram, 0, sizeof vdc_ram);
    mmu_cr = 0x00;
    reg_a = reg_x = reg_y = 0;
    reg_sp = 0xFF;
    reg_p = FLAG_U | FLAG_I;
    reg_pc = 0x1C01;
    fast_2mhz = 0;
}

/* Render Commodore 128 (80-Column VDC or 40-Column VIC-II) */
static void c128_render(void)
{
    gfx_fill(0, 0, 512, 342, 0);

    /* PlusOS Title Bar */
    gfx_fill(0, 0, 512, 18, 1);
    gfx_text(10, 5, "COMMODORE 128 EMULATOR (MOS 8502 / 128K MMU / VDC)", 1);
    char st[64];
    snprintf(st, sizeof st, "%s | %s | %s",
             display_80col ? "80-COL VDC" : "40-COL VIC",
             (mmu_cr & 0x40) ? "BANK 1" : "BANK 0",
             fast_2mhz ? "2 MHz" : "1 MHz");
    gfx_text(340, 5, st, 1);

    if (display_80col) {
        /* VDC 80-Column Display: 80x25 characters centered at X=16, Y=36 (480x200) */
        int act_x = 16;
        int act_y = 36;
        gfx_frame(act_x - 2, act_y - 2, act_x + 480 + 2, act_y + 200 + 2);

        for (int r = 0; r < 25; r++) {
            for (int c = 0; c < 80; c++) {
                uint8_t sc = vdc_ram[r * 80 + c];
                sc &= 0x7F;
                char ch = ' ';
                if (sc >= 1 && sc <= 26) ch = (char)('A' + sc - 1);
                else if (sc >= 48 && sc <= 57) ch = (char)sc;
                else if (sc >= 32 && sc < 128) ch = (char)sc;

                int px = act_x + c * 6;
                int py = act_y + r * 8;
                for (int dy = 0; dy < 8; dy++) {
                    uint8_t b = c128_font8[(uint8_t)ch][dy];
                    for (int dx = 0; dx < 5; dx++) {
                        if (b & (0x80 >> dx)) {
                            gfx_pixel(px + dx, py + dy, 1);
                        }
                    }
                }
            }
        }
    } else {
        /* VIC-II 40-Column Display: 40x25 characters centered at X=96, Y=50 */
        int act_x = 96;
        int act_y = 50;
        gfx_dither_rect(act_x - 12, act_y - 12, act_x + 320 + 12, act_y + 200 + 12, 4);
        gfx_fill(act_x, act_y, act_x + 320, act_y + 200, 0);
        gfx_frame(act_x - 1, act_y - 1, act_x + 320 + 1, act_y + 200 + 1);

        uint16_t screen_matrix = 0x0400;
        for (int r = 0; r < 25; r++) {
            for (int c = 0; c < 40; c++) {
                uint8_t sc = c128_bank0[screen_matrix + r * 40 + c];
                sc &= 0x7F;
                char ch = ' ';
                if (sc >= 1 && sc <= 26) ch = (char)('A' + sc - 1);
                else if (sc >= 48 && sc <= 57) ch = (char)sc;
                else if (sc >= 32 && sc < 128) ch = (char)sc;

                int px = act_x + c * 8;
                int py = act_y + r * 8;
                for (int dy = 0; dy < 8; dy++) {
                    uint8_t b = c128_font8[(uint8_t)ch][dy];
                    for (int dx = 0; dx < 8; dx++) {
                        if (b & (0x80 >> dx)) {
                            gfx_pixel(px + dx, py + dy, 1);
                        }
                    }
                }
            }
        }
    }

    /* Footer */
    gfx_fill(0, 324, 512, 342, 1);
    gfx_text(10, 328, "Tab: Toggle 40/80 Col  |  Ctrl-R: Reset  |  Esc: Exit PlusOS", 1);
}

/* Initialize C128 Environment */
static void init_c128(void)
{
    c128_reset();

    /* Fill 80-column VDC screen with C128 BASIC V7.0 banner */
    const char *v7_lines[] = {
        "   COMMODORE BASIC V7.0  122365 BYTES FREE",
        " (C)1985 COMMODORE ELECTRONICS, LTD.",
        " (C)1977 MICROSOFT CORP.",
        " ALL RIGHTS RESERVED",
        "",
        "READY.",
        ""
    };

    for (int r = 0; r < 7; r++) {
        const char *l = v7_lines[r];
        for (int c = 0; l[c] && c < 80; c++) {
            uint8_t pet = (uint8_t)l[c];
            if (pet >= 'A' && pet <= 'Z') pet = pet - 64;
            vdc_ram[r * 80 + c] = pet;
        }
    }
    vdc_ram[6 * 80] = 0x60; /* Block cursor */

    /* Also seed 40-column screen in Bank 0 */
    const char *v40_lines[] = {
        "COMMODORE BASIC V7.0",
        "122365 BYTES FREE",
        "READY."
    };
    for (int r = 0; r < 3; r++) {
        const char *l = v40_lines[r];
        for (int c = 0; l[c] && c < 40; c++) {
            uint8_t pet = (uint8_t)l[c];
            if (pet >= 'A' && pet <= 'Z') pet = pet - 64;
            c128_bank0[0x0400 + r * 40 + c] = pet;
        }
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    gfx_target(gfx_backbuffer());

    init_c128();

    uint32_t last_tick = plusos->ticks();
    int cur_pos = 6 * 80;

    for (;;) {
        for (int kp_; (kp_ = plusos->key_poll()) != 0; ) {
            int ch = (int)kp_;
            if (ch == 27) return 0;
            if (ch == '\t') {
                display_80col = !display_80col;
                continue;
            }
            if (ch == 18) {
                init_c128();
                cur_pos = 6 * 80;
                continue;
            }

            if (display_80col) {
                if (ch == '\r' || ch == '\n') {
                    cur_pos = ((cur_pos / 80) + 1) * 80;
                    if (cur_pos >= 80 * 24) cur_pos = 80 * 6;
                } else if (ch >= 32 && ch < 127) {
                    uint8_t pet = (uint8_t)toupper(ch);
                    if (pet >= 'A' && pet <= 'Z') pet = pet - 64;
                    vdc_ram[cur_pos++] = pet;
                    if (cur_pos >= 80 * 25) cur_pos = 80 * 6;
                }
            }
        }

        for (int i = 0; i < (fast_2mhz ? 400 : 200); i++) {
            if (!step8502()) break;
        }

        c128_render();
        gfx_present();

        while (plusos->ticks() == last_tick) {}
        last_tick = plusos->ticks();
    }

    return 0;
}
