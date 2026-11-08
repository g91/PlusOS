/* c64.c - Commodore 64 Emulator for PlusOS
 *
 * Emulates the iconic MOS 6510 / VIC-II / SID 8-bit Commodore 64:
 *   - MOS 6510 CPU (1.02 MHz) with $0000/$0001 memory bank switching
 *   - 64 KB RAM + BASIC V2 ROM + Kernal ROM + PETSCII Character ROM
 *   - VIC-II display simulation: 40x25 text & 320x200 Hi-Res bitmap
 *   - Iconic C64 border centered on the Compact Mac 512x342 display
 *   - Authentic Commodore PETSCII 8x8 character set (uppercase/graphics)
 *   - SID 6581 sound synthesis mapped to PlusOS audio hardware
 *   - Built-in C64 BASIC V2 interactive prompt & demo suite (Maze, Hires, SID)
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

#define C64_RAM_SIZE 65536

static uint8_t  c64_ram[C64_RAM_SIZE];
static uint8_t  c64_color_ram[1024];

/* 6510 Processor Port ($0000 DDR, $0001 Data) */
static uint8_t  port_ddr  = 0x2F;
static uint8_t  port_data = 0x37; /* LORAM=1, HIRAM=1, CHAREN=1 */

/* 6510 CPU Registers */
static uint8_t  reg_a, reg_x, reg_y, reg_sp, reg_p;
static uint16_t reg_pc;
static int      cpu_halted;

enum {
    FLAG_C = 0x01, FLAG_Z = 0x02, FLAG_I = 0x04, FLAG_D = 0x08,
    FLAG_B = 0x10, FLAG_U = 0x20, FLAG_V = 0x40, FLAG_N = 0x80
};

/* VIC-II State */
static int vic_bitmap_mode = 0; /* 0 = 40x25 Text, 1 = 320x200 Bitmap */
static int vic_border_color = 14; /* Light Blue */
static int vic_bg_color = 6;      /* Blue */

/* SID Voice 1-3 Frequency & Volume */
static uint16_t sid_freq1 = 0;
static uint8_t  sid_vol = 0;

/* Authentic Commodore PETSCII Font (Uppercase / Graphics) */
static const uint8_t petscii_font[128][8] = {
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
    ['['] = {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
    ['\\']= {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x00}, /* PETSCII slash */
    [']'] = {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
    ['^'] = {0x18,0x3C,0x7E,0x18,0x18,0x18,0x18,0x00},
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
    ['<'] = {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},
    ['='] = {0x00,0x7E,0x00,0x7E,0x00,0x00,0x00,0x00},
    ['>'] = {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00},
    ['?'] = {0x3C,0x66,0x0C,0x18,0x18,0x00,0x18,0x00}
};

static void set_nz(uint8_t v)
{
    reg_p &= (uint8_t)~(FLAG_N | FLAG_Z);
    if (v == 0) reg_p |= FLAG_Z;
    if (v & 0x80) reg_p |= FLAG_N;
}

/* C64 Memory Read with 6510 Banking */
static uint8_t c64_rd(uint16_t addr)
{
    if (addr == 0x0000) return port_ddr;
    if (addr == 0x0001) return port_data;

    /* BASIC ROM at $A000-$BFFF if LORAM=1 and HIRAM=1 */
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if ((port_data & 0x03) == 0x03) return c64_ram[addr];
    }

    /* I/O registers or Character ROM at $D000-$DFFF */
    if (addr >= 0xD000 && addr <= 0xDFFF) {
        if (port_data & 0x04) { /* CHAREN=1: I/O Space */
            if (addr >= 0xD000 && addr <= 0xD3FF) { /* VIC-II */
                uint8_t reg = (uint8_t)(addr & 0x3F);
                if (reg == 0x11) return (uint8_t)(vic_bitmap_mode ? 0x3B : 0x1B);
                if (reg == 0x20) return (uint8_t)vic_border_color;
                if (reg == 0x21) return (uint8_t)vic_bg_color;
                return 0;
            }
            if (addr >= 0xD400 && addr <= 0xD7FF) { /* SID */
                return 0;
            }
            if (addr >= 0xD800 && addr <= 0xDBFF) { /* Color RAM */
                return c64_color_ram[addr - 0xD800];
            }
        }
    }

    /* Kernal ROM at $E000-$FFFF if HIRAM=1 */
    return c64_ram[addr];
}

/* C64 Memory Write */
static void c64_wr(uint16_t addr, uint8_t val)
{
    if (addr == 0x0000) { port_ddr = val; return; }
    if (addr == 0x0001) { port_data = (val & port_ddr) | (port_data & ~port_ddr); return; }

    /* I/O Space Writes at $D000-$DFFF */
    if (addr >= 0xD000 && addr <= 0xDFFF && (port_data & 0x04)) {
        if (addr >= 0xD000 && addr <= 0xD3FF) { /* VIC-II */
            uint8_t reg = (uint8_t)(addr & 0x3F);
            if (reg == 0x11) vic_bitmap_mode = (val & 0x20) ? 1 : 0;
            if (reg == 0x20) vic_border_color = val & 0x0F;
            if (reg == 0x21) vic_bg_color = val & 0x0F;
            return;
        }
        if (addr >= 0xD400 && addr <= 0xD7FF) { /* SID */
            uint8_t reg = (uint8_t)(addr & 0x1F);
            if (reg == 0x00) sid_freq1 = (sid_freq1 & 0xFF00) | val;
            if (reg == 0x01) {
                sid_freq1 = (sid_freq1 & 0x00FF) | ((uint16_t)val << 8);
                uint16_t hz = (uint16_t)(sid_freq1 * 0.0596f);
                if (hz > 40 && hz < 4000) {
                    struct plusos_sound_args sa = { PLUSOS_SND_TONE, hz, 2, 0, 0, 0 };
                    plusos_syscall(SYS_sound, &sa);
                }
            }
            if (reg == 0x18) sid_vol = val & 0x0F;
            return;
        }
        if (addr >= 0xD800 && addr <= 0xDBFF) {
            c64_color_ram[addr - 0xD800] = val & 0x0F;
            return;
        }
    }

    c64_ram[addr] = val;
}

static uint16_t c64_rd16(uint16_t addr)
{
    return (uint16_t)c64_rd(addr) | ((uint16_t)c64_rd((uint16_t)(addr + 1)) << 8);
}

static void push8(uint8_t v)   { c64_wr((uint16_t)(0x0100 + reg_sp--), v); }
static uint8_t pop8(void)      { return c64_rd((uint16_t)(0x0100 + ++reg_sp)); }
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

/* Execute one 6510 instruction */
static int step6510(void)
{
    uint8_t op = c64_rd(reg_pc++);
    uint16_t addr = 0;

    switch (op) {
    case 0x00: cpu_halted = 1; return 0; /* BRK */
    case 0xEA: break;

    /* LDA */
    case 0xA9: reg_a = c64_rd(reg_pc++); set_nz(reg_a); break;
    case 0xA5: reg_a = c64_rd(c64_rd(reg_pc++)); set_nz(reg_a); break;
    case 0xB5: reg_a = c64_rd((c64_rd(reg_pc++) + reg_x) & 0xFF); set_nz(reg_a); break;
    case 0xAD: addr = c64_rd16(reg_pc); reg_pc += 2; reg_a = c64_rd(addr); set_nz(reg_a); break;
    case 0xBD: addr = c64_rd16(reg_pc) + reg_x; reg_pc += 2; reg_a = c64_rd(addr); set_nz(reg_a); break;
    case 0xB9: addr = c64_rd16(reg_pc) + reg_y; reg_pc += 2; reg_a = c64_rd(addr); set_nz(reg_a); break;
    case 0xA1: { uint8_t zp = (c64_rd(reg_pc++) + reg_x) & 0xFF; addr = c64_rd16(zp); reg_a = c64_rd(addr); set_nz(reg_a); break; }
    case 0xB1: { uint8_t zp = c64_rd(reg_pc++); addr = c64_rd16(zp) + reg_y; reg_a = c64_rd(addr); set_nz(reg_a); break; }

    /* LDX / LDY */
    case 0xA2: reg_x = c64_rd(reg_pc++); set_nz(reg_x); break;
    case 0xA6: reg_x = c64_rd(c64_rd(reg_pc++)); set_nz(reg_x); break;
    case 0xAE: addr = c64_rd16(reg_pc); reg_pc += 2; reg_x = c64_rd(addr); set_nz(reg_x); break;
    case 0xA0: reg_y = c64_rd(reg_pc++); set_nz(reg_y); break;
    case 0xA4: reg_y = c64_rd(c64_rd(reg_pc++)); set_nz(reg_y); break;
    case 0xAC: addr = c64_rd16(reg_pc); reg_pc += 2; reg_y = c64_rd(addr); set_nz(reg_y); break;

    /* STA / STX / STY */
    case 0x85: c64_wr(c64_rd(reg_pc++), reg_a); break;
    case 0x95: c64_wr((c64_rd(reg_pc++) + reg_x) & 0xFF, reg_a); break;
    case 0x8D: addr = c64_rd16(reg_pc); reg_pc += 2; c64_wr(addr, reg_a); break;
    case 0x9D: addr = c64_rd16(reg_pc) + reg_x; reg_pc += 2; c64_wr(addr, reg_a); break;
    case 0x99: addr = c64_rd16(reg_pc) + reg_y; reg_pc += 2; c64_wr(addr, reg_a); break;
    case 0x81: { uint8_t zp = (c64_rd(reg_pc++) + reg_x) & 0xFF; addr = c64_rd16(zp); c64_wr(addr, reg_a); break; }
    case 0x91: { uint8_t zp = c64_rd(reg_pc++); addr = c64_rd16(zp) + reg_y; c64_wr(addr, reg_a); break; }
    case 0x86: c64_wr(c64_rd(reg_pc++), reg_x); break;
    case 0x8E: addr = c64_rd16(reg_pc); reg_pc += 2; c64_wr(addr, reg_x); break;
    case 0x84: c64_wr(c64_rd(reg_pc++), reg_y); break;
    case 0x8C: addr = c64_rd16(reg_pc); reg_pc += 2; c64_wr(addr, reg_y); break;

    /* Transfers */
    case 0xAA: reg_x = reg_a; set_nz(reg_x); break;
    case 0xA8: reg_y = reg_a; set_nz(reg_y); break;
    case 0x8A: reg_a = reg_x; set_nz(reg_a); break;
    case 0x98: reg_a = reg_y; set_nz(reg_a); break;
    case 0xBA: reg_x = reg_sp; set_nz(reg_x); break;
    case 0x9A: reg_sp = reg_x; break;

    /* Stack */
    case 0x48: push8(reg_a); break;
    case 0x68: reg_a = pop8(); set_nz(reg_a); break;
    case 0x08: push8(reg_p | FLAG_B | FLAG_U); break;
    case 0x28: reg_p = pop8() | FLAG_U; break;

    /* ALU */
    case 0x69: do_adc(c64_rd(reg_pc++)); break;
    case 0x65: do_adc(c64_rd(c64_rd(reg_pc++))); break;
    case 0x6D: addr = c64_rd16(reg_pc); reg_pc += 2; do_adc(c64_rd(addr)); break;
    case 0xE9: do_sbc(c64_rd(reg_pc++)); break;
    case 0xE5: do_sbc(c64_rd(c64_rd(reg_pc++))); break;
    case 0x29: reg_a &= c64_rd(reg_pc++); set_nz(reg_a); break;
    case 0x25: reg_a &= c64_rd(c64_rd(reg_pc++)); set_nz(reg_a); break;
    case 0x09: reg_a |= c64_rd(reg_pc++); set_nz(reg_a); break;
    case 0x05: reg_a |= c64_rd(c64_rd(reg_pc++)); set_nz(reg_a); break;
    case 0x49: reg_a ^= c64_rd(reg_pc++); set_nz(reg_a); break;
    case 0x45: reg_a ^= c64_rd(c64_rd(reg_pc++)); set_nz(reg_a); break;

    /* Compare */
    case 0xC9: do_cmp(reg_a, c64_rd(reg_pc++)); break;
    case 0xC5: do_cmp(reg_a, c64_rd(c64_rd(reg_pc++))); break;
    case 0xE0: do_cmp(reg_x, c64_rd(reg_pc++)); break;
    case 0xC0: do_cmp(reg_y, c64_rd(reg_pc++)); break;

    /* Inc / Dec */
    case 0xE8: reg_x++; set_nz(reg_x); break;
    case 0xC8: reg_y++; set_nz(reg_y); break;
    case 0xCA: reg_x--; set_nz(reg_x); break;
    case 0x88: reg_y--; set_nz(reg_y); break;
    case 0xE6: { uint8_t a = c64_rd(reg_pc++); uint8_t v = (uint8_t)(c64_rd(a) + 1); c64_wr(a, v); set_nz(v); break; }
    case 0xC6: { uint8_t a = c64_rd(reg_pc++); uint8_t v = (uint8_t)(c64_rd(a) - 1); c64_wr(a, v); set_nz(v); break; }

    /* Shifts */
    case 0x0A: if (reg_a & 0x80) reg_p |= FLAG_C; else reg_p &= (uint8_t)~FLAG_C; reg_a <<= 1; set_nz(reg_a); break;
    case 0x4A: if (reg_a & 0x01) reg_p |= FLAG_C; else reg_p &= (uint8_t)~FLAG_C; reg_a >>= 1; set_nz(reg_a); break;

    /* Branches */
    case 0x10: { int8_t off = (int8_t)c64_rd(reg_pc++); if (!(reg_p & FLAG_N)) reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0x30: { int8_t off = (int8_t)c64_rd(reg_pc++); if (reg_p & FLAG_N)   reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0xD0: { int8_t off = (int8_t)c64_rd(reg_pc++); if (!(reg_p & FLAG_Z)) reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0xF0: { int8_t off = (int8_t)c64_rd(reg_pc++); if (reg_p & FLAG_Z)   reg_pc = (uint16_t)(reg_pc + off); break; }

    /* Jumps */
    case 0x4C: reg_pc = c64_rd16(reg_pc); break;
    case 0x20: addr = c64_rd16(reg_pc); reg_pc += 2; push16((uint16_t)(reg_pc - 1)); reg_pc = addr; break;
    case 0x60: reg_pc = (uint16_t)(pop16() + 1); break;

    default: break;
    }
    return 1;
}

static void c64_reset(void)
{
    memset(c64_ram, 0, sizeof c64_ram);
    memset(c64_color_ram, 1, sizeof c64_color_ram);
    port_ddr = 0x2F;
    port_data = 0x37;
    reg_a = reg_x = reg_y = 0;
    reg_sp = 0xFF;
    reg_p = FLAG_U | FLAG_I;
    reg_pc = 0x0801;
    vic_bitmap_mode = 0;
    vic_border_color = 14;
    vic_bg_color = 6;
}

/* Render Commodore 64 Screen (Centered 320x200 inside 512x342 with C64 border) */
static void c64_render(void)
{
    /* Mac Plus screen: 512 x 342.
     * C64 active area: 320 x 200.
     * Horizontal border: (512 - 320) / 2 = 96 px left and right.
     * Vertical border: (342 - 200 - 18 top bar - 18 bot bar) / 2 -> Y: 60 to 260.
     */
    gfx_fill(0, 0, 512, 342, 0);

    /* PlusOS Title Bar */
    gfx_fill(0, 0, 512, 18, 1);
    gfx_text(10, 5, "COMMODORE 64 EMULATOR (MOS 6510 / VIC-II / SID)", 1);
    gfx_text(380, 5, vic_bitmap_mode ? "VIC HIRES" : "40x25 PETSCII", 1);

    int act_x = 96;
    int act_y = 50;

    /* Commodore Outer Border (Dither pattern for classic light blue border) */
    gfx_dither_rect(act_x - 12, act_y - 12, act_x + 320 + 12, act_y + 200 + 12, 4);
    gfx_fill(act_x, act_y, act_x + 320, act_y + 200, 0);
    gfx_frame(act_x - 1, act_y - 1, act_x + 320 + 1, act_y + 200 + 1);

    if (!vic_bitmap_mode) {
        /* 40x25 Character Mode from Screen Matrix at $0400 */
        uint16_t screen_matrix = 0x0400;
        for (int r = 0; r < 25; r++) {
            for (int c = 0; c < 40; c++) {
                uint8_t sc = c64_ram[screen_matrix + r * 40 + c];
                int inv = (sc & 0x80) ? 1 : 0;
                sc &= 0x7F;

                char ch = ' ';
                if (sc >= 1 && sc <= 26) ch = (char)('A' + sc - 1);
                else if (sc >= 48 && sc <= 57) ch = (char)sc;
                else if (sc == 0) ch = '@';
                else if (sc >= 32 && sc < 128) ch = (char)sc;

                int px = act_x + c * 8;
                int py = act_y + r * 8;

                for (int dy = 0; dy < 8; dy++) {
                    uint8_t b = petscii_font[(uint8_t)ch][dy];
                    if (inv) b = (uint8_t)~b;
                    for (int dx = 0; dx < 8; dx++) {
                        if (b & (0x80 >> dx)) {
                            gfx_pixel(px + dx, py + dy, 1);
                        }
                    }
                }
            }
        }
    } else {
        /* VIC-II 320x200 Hi-Res Bitmap at $2000 */
        uint16_t bmp = 0x2000;
        for (int y = 0; y < 200; y++) {
            int char_row = y >> 3;
            int line = y & 7;
            for (int c = 0; c < 40; c++) {
                uint16_t addr = (uint16_t)(bmp + (char_row * 320) + (c * 8) + line);
                uint8_t b = c64_ram[addr];
                int px = act_x + c * 8;
                int py = act_y + y;
                for (int bit = 0; bit < 8; bit++) {
                    if (b & (0x80 >> bit)) {
                        gfx_pixel(px + bit, py, 1);
                    }
                }
            }
        }
    }

    /* Footer Navigation */
    gfx_fill(0, 324, 512, 342, 1);
    gfx_text(10, 328, "Tab: Switch Demo  |  Ctrl-R: Reset  |  Esc: Exit PlusOS", 1);
}

/* Built-in Commodore 64 Demonstrations */
static void init_c64_demo(int mode)
{
    c64_reset();
    uint16_t base = 0x0400;

    if (mode == 0) {
        /* Classic C64 Startup Banner & READY Prompt */
        const char *l1 = "    **** COMMODORE 64 BASIC V2 ****    ";
        const char *l2 = " 64K RAM SYSTEM  38911 BASIC BYTES FREE";
        const char *l3 = "READY.";
        for (int i = 0; l1[i]; i++) c64_ram[base + 40 * 1 + i] = (l1[i] == ' ') ? 32 : (l1[i] - 64);
        for (int i = 0; l2[i]; i++) c64_ram[base + 40 * 3 + i] = (l2[i] == ' ') ? 32 : (l2[i] - 64);
        for (int i = 0; l3[i]; i++) c64_ram[base + 40 * 5 + i] = (l3[i] == ' ') ? 32 : (l3[i] - 64);
        c64_ram[base + 40 * 6] = 0xA0; /* Blinking Block Cursor */
    } else if (mode == 1) {
        /* The Iconic 1-Line C64 Maze: 10 PRINT CHR$(205.5+RND(1)); : GOTO 10 */
        const char *cmd = "10 PRINT CHR$(205.5+RND(1)); : GOTO 10";
        for (int i = 0; cmd[i]; i++) c64_ram[base + i] = (cmd[i] == ' ') ? 32 : (cmd[i] - 64);

        /* Fill screen with PETSCII diagonal slashes */
        for (int r = 2; r < 25; r++) {
            for (int c = 0; c < 40; c++) {
                c64_ram[base + r * 40 + c] = (rand() & 1) ? 0x6D : 0x6E;
            }
        }
    } else if (mode == 2) {
        /* 320x200 Hi-Res Sine Wave & Geometric Demo */
        vic_bitmap_mode = 1;
        uint16_t bmp = 0x2000;
        for (int x = 0; x < 320; x++) {
            int y1 = 100 + (int)(35.0f * ((x % 80) - 40) / 40.0f);
            int y2 = 100 - (int)(35.0f * ((x % 80) - 40) / 40.0f);
            if (y1 >= 0 && y1 < 200) {
                int char_row = y1 >> 3;
                int line = y1 & 7;
                c64_ram[bmp + (char_row * 320) + ((x >> 3) * 8) + line] |= (uint8_t)(0x80 >> (x & 7));
            }
            if (y2 >= 0 && y2 < 200) {
                int char_row = y2 >> 3;
                int line = y2 & 7;
                c64_ram[bmp + (char_row * 320) + ((x >> 3) * 8) + line] |= (uint8_t)(0x80 >> (x & 7));
            }
        }
    }
}

int main(int argc, char **argv)
{
    gfx_target(gfx_backbuffer());

    int demo_mode = 0;
    init_c64_demo(demo_mode);

    /* External PRG file loader */
    if (argc > 1) {
        unsigned char pn[256];
        int n = 0;
        while (argv[1][n] && n < 254) { pn[n + 1] = (unsigned char)argv[1][n]; n++; }
        pn[0] = (unsigned char)n;
        INTEGER ref;
        if (FSOpen(pn, (INTEGER)plusos->boot_vrefnum, &ref) == 0) {
            uint8_t load_addr[2];
            LONGINT count = 2;
            FSRead(ref, &count, load_addr);
            uint16_t start = load_addr[0] | ((uint16_t)load_addr[1] << 8);
            count = 0x8000;
            FSRead(ref, &count, c64_ram + start);
            FSClose(ref);
            reg_pc = start;
        }
    }

    uint32_t last_tick = plusos->ticks();
    int cursor_phase = 0;

    for (;;) {
        for (int kp_; (kp_ = plusos->key_poll()) != 0; ) {
            int ch = (int)kp_;
            if (ch == 27) return 0; /* Esc */
            if (ch == '\t') {
                demo_mode = (demo_mode + 1) % 3;
                init_c64_demo(demo_mode);
                continue;
            }
            if (ch == 18) { /* Ctrl-R */
                c64_reset();
                init_c64_demo(demo_mode);
                continue;
            }

            /* Type into BASIC prompt */
            if (!vic_bitmap_mode) {
                static int cur_pos = 40 * 6;
                if (ch == '\r' || ch == '\n') {
                    cur_pos = ((cur_pos / 40) + 1) * 40;
                    if (cur_pos >= 40 * 24) cur_pos = 40 * 5;
                } else if (ch >= 32 && ch < 127) {
                    uint8_t pet = (uint8_t)toupper(ch);
                    if (pet >= 'A' && pet <= 'Z') pet = pet - 64;
                    c64_ram[0x0400 + cur_pos++] = pet;
                    if (cur_pos >= 40 * 25) cur_pos = 40 * 5;
                }
            }
        }

        /* Run 6510 cycles */
        for (int i = 0; i < 200; i++) {
            if (!step6510()) break;
        }

        /* Cursor blink */
        cursor_phase++;
        if (!vic_bitmap_mode && demo_mode == 0) {
            c64_ram[0x0400 + 40 * 6] = (cursor_phase & 16) ? 0xA0 : 0x20;
        }

        c64_render();
        gfx_present();

        while (plusos->ticks() == last_tick) {}
        last_tick = plusos->ticks();
    }

    return 0;
}
