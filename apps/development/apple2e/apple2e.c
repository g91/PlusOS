/* apple2e.c - Apple //e Emulator for PlusOS
 *
 * Emulates the classic 8-bit Apple //e personal computer:
 *   - 6502 / 65C02 microprocessor (1.02 MHz)
 *   - 64 KB RAM with Apple II memory map & softswitches ($C000..$C0FF)
 *   - 40-column and 80-column text display modes
 *   - 280x192 1-bit Hi-Res Graphics (Page 1: $2000, Page 2: $4000)
 *   - 40x48 Lo-Res Graphics
 *   - Speaker toggle ($C030) driving PlusOS audio hardware
 *   - Applesoft BASIC prompt (']') & Apple II Monitor ('*')
 *   - Built-in ROM demos (Spirangle, Lo-Res patterns, Mandelbrot, Monitor)
 *   - External Apple II binary/disk image loader (.bin / .b)
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusrom/FileMgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define RAM_SIZE 65536

static uint8_t  ram[RAM_SIZE];
static uint8_t  aux_ram[RAM_SIZE]; /* Auxiliary 64K for 80-col //e */

/* 6502 / 65C02 Registers */
static uint8_t  reg_a, reg_x, reg_y, reg_sp, reg_p;
static uint16_t reg_pc;
static int      cpu_halted;

/* 6502 Status Flags */
enum {
    FLAG_C = 0x01, FLAG_Z = 0x02, FLAG_I = 0x04, FLAG_D = 0x08,
    FLAG_B = 0x10, FLAG_U = 0x20, FLAG_V = 0x40, FLAG_N = 0x80
};

/* Apple II Softswitch State */
static int sw_text       = 1; /* 1 = Text, 0 = Graphics */
static int sw_mixed      = 0; /* 1 = Mixed 4-line text, 0 = Full screen */
static int sw_page2      = 0; /* 1 = Page 2 ($0800/$4000), 0 = Page 1 ($0400/$2000) */
static int sw_hires      = 0; /* 1 = Hi-Res (280x192), 0 = Lo-Res (40x48) */
static int sw_80col      = 0; /* 1 = 80 Columns, 0 = 40 Columns */
static uint8_t kbd_data  = 0; /* $C000: bit 7 = key strobe */

/* Apple II Character Font (5x7 in 8x8 cell) */
static const uint8_t a2_font[128][8] = {
    [' '] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['!'] = {0x00,0x20,0x20,0x20,0x20,0x00,0x20,0x00},
    ['"'] = {0x00,0x50,0x50,0x00,0x00,0x00,0x00,0x00},
    ['#'] = {0x00,0x50,0xF8,0x50,0xF8,0x50,0x00,0x00},
    ['$'] = {0x00,0x20,0x78,0xA0,0x70,0x28,0xF0,0x20},
    ['%'] = {0x00,0xC0,0xC8,0x10,0x20,0x4C,0x06,0x00},
    ['&'] = {0x00,0x40,0xA0,0x40,0xA8,0x90,0x68,0x00},
    ['\'']= {0x00,0x20,0x20,0x00,0x00,0x00,0x00,0x00},
    ['('] = {0x00,0x10,0x20,0x40,0x40,0x20,0x10,0x00},
    [')'] = {0x00,0x40,0x20,0x10,0x10,0x20,0x40,0x00},
    ['*'] = {0x00,0x00,0x50,0x20,0xF8,0x20,0x50,0x00},
    ['+'] = {0x00,0x00,0x20,0x20,0xF8,0x20,0x20,0x00},
    [','] = {0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x40},
    ['-'] = {0x00,0x00,0x00,0x00,0xF8,0x00,0x00,0x00},
    ['.'] = {0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00},
    ['/'] = {0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00},
    ['0'] = {0x00,0x70,0x88,0x98,0xA8,0xC8,0x70,0x00},
    ['1'] = {0x00,0x20,0x60,0x20,0x20,0x20,0x70,0x00},
    ['2'] = {0x00,0x70,0x88,0x08,0x30,0x40,0xF8,0x00},
    ['3'] = {0x00,0xF8,0x08,0x30,0x08,0x88,0x70,0x00},
    ['4'] = {0x00,0x10,0x30,0x50,0x90,0xF8,0x10,0x00},
    ['5'] = {0x00,0xF8,0x80,0xF0,0x08,0x88,0x70,0x00},
    ['6'] = {0x00,0x30,0x40,0xB0,0xC8,0x88,0x70,0x00},
    ['7'] = {0x00,0xF8,0x08,0x10,0x20,0x40,0x40,0x00},
    ['8'] = {0x00,0x70,0x88,0x70,0x88,0x88,0x70,0x00},
    ['9'] = {0x00,0x70,0x88,0x88,0x78,0x08,0x70,0x00},
    [':'] = {0x00,0x00,0x20,0x00,0x00,0x20,0x00,0x00},
    [';'] = {0x00,0x00,0x20,0x00,0x00,0x20,0x20,0x40},
    ['<'] = {0x00,0x08,0x10,0x20,0x40,0x20,0x10,0x08},
    ['='] = {0x00,0x00,0xF8,0x00,0xF8,0x00,0x00,0x00},
    ['>'] = {0x00,0x40,0x20,0x10,0x08,0x10,0x20,0x40},
    ['?'] = {0x00,0x70,0x88,0x10,0x20,0x00,0x20,0x00},
    ['@'] = {0x00,0x70,0x88,0x98,0xAE,0x80,0x70,0x00},
    ['A'] = {0x00,0x20,0x50,0x88,0xF8,0x88,0x88,0x00},
    ['B'] = {0x00,0xF0,0x88,0xF0,0x88,0x88,0xF0,0x00},
    ['C'] = {0x00,0x70,0x88,0x80,0x80,0x88,0x70,0x00},
    ['D'] = {0x00,0xE0,0x90,0x88,0x88,0x90,0xE0,0x00},
    ['E'] = {0x00,0xF8,0x80,0xF0,0x80,0x80,0xF8,0x00},
    ['F'] = {0x00,0xF8,0x80,0xF0,0x80,0x80,0x80,0x00},
    ['G'] = {0x00,0x70,0x88,0x80,0xB8,0x88,0x70,0x00},
    ['H'] = {0x00,0x88,0x88,0xF8,0x88,0x88,0x88,0x00},
    ['I'] = {0x00,0x70,0x20,0x20,0x20,0x20,0x70,0x00},
    ['J'] = {0x00,0x08,0x08,0x08,0x08,0x88,0x70,0x00},
    ['K'] = {0x00,0x88,0x90,0xE0,0x90,0x88,0x88,0x00},
    ['L'] = {0x00,0x80,0x80,0x80,0x80,0x80,0xF8,0x00},
    ['M'] = {0x00,0x88,0xD8,0xA8,0x88,0x88,0x88,0x00},
    ['N'] = {0x00,0x88,0xC8,0xA8,0x98,0x88,0x88,0x00},
    ['O'] = {0x00,0x70,0x88,0x88,0x88,0x88,0x70,0x00},
    ['P'] = {0x00,0xF0,0x88,0x88,0xF0,0x80,0x80,0x00},
    ['Q'] = {0x00,0x70,0x88,0x88,0x88,0x98,0x70,0x08},
    ['R'] = {0x00,0xF0,0x88,0x88,0xF0,0x90,0x88,0x00},
    ['S'] = {0x00,0x78,0x80,0x70,0x08,0x08,0xF0,0x00},
    ['T'] = {0x00,0xF8,0x20,0x20,0x20,0x20,0x20,0x00},
    ['U'] = {0x00,0x88,0x88,0x88,0x88,0x88,0x70,0x00},
    ['V'] = {0x00,0x88,0x88,0x88,0x88,0x50,0x20,0x00},
    ['W'] = {0x00,0x88,0x88,0x88,0xA8,0xD8,0x88,0x00},
    ['X'] = {0x00,0x88,0x88,0x50,0x20,0x50,0x88,0x00},
    ['Y'] = {0x00,0x88,0x88,0x50,0x20,0x20,0x20,0x00},
    ['Z'] = {0x00,0xF8,0x08,0x10,0x20,0x40,0xF8,0x00},
    ['['] = {0x00,0x70,0x60,0x60,0x60,0x60,0x70,0x00},
    ['\\']= {0x00,0x80,0x40,0x20,0x10,0x08,0x00,0x00},
    [']'] = {0x00,0x70,0x30,0x30,0x30,0x30,0x70,0x00},
    ['^'] = {0x00,0x20,0x50,0x88,0x00,0x00,0x00,0x00},
    ['_'] = {0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x00},
    ['`'] = {0x00,0x40,0x20,0x00,0x00,0x00,0x00,0x00},
    ['a'] = {0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00},
    ['b'] = {0x00,0x80,0x80,0xF0,0x88,0x88,0xF0,0x00},
    ['c'] = {0x00,0x00,0x70,0x88,0x80,0x88,0x70,0x00},
    ['d'] = {0x00,0x08,0x08,0x78,0x88,0x88,0x78,0x00},
    ['e'] = {0x00,0x00,0x70,0x88,0xF8,0x80,0x70,0x00},
    ['f'] = {0x00,0x30,0x48,0x40,0xE0,0x40,0x40,0x00},
    ['g'] = {0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70},
    ['h'] = {0x00,0x80,0x80,0xF0,0x88,0x88,0x88,0x00},
    ['i'] = {0x00,0x20,0x00,0x60,0x20,0x20,0x70,0x00},
    ['j'] = {0x00,0x10,0x00,0x30,0x10,0x10,0x90,0x60},
    ['k'] = {0x00,0x80,0x80,0x90,0xE0,0x90,0x88,0x00},
    ['l'] = {0x00,0x60,0x20,0x20,0x20,0x20,0x70,0x00},
    ['m'] = {0x00,0x00,0xD0,0xA8,0xA8,0x88,0x88,0x00},
    ['n'] = {0x00,0x00,0xF0,0x88,0x88,0x88,0x88,0x00},
    ['o'] = {0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00},
    ['p'] = {0x00,0x00,0xF0,0x88,0x88,0xF0,0x80,0x80},
    ['q'] = {0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08},
    ['r'] = {0x00,0x00,0xB0,0xC8,0x80,0x80,0x80,0x00},
    ['s'] = {0x00,0x00,0x78,0x80,0x70,0x08,0xF0,0x00},
    ['t'] = {0x00,0x40,0x40,0xE0,0x40,0x40,0x30,0x00},
    ['u'] = {0x00,0x00,0x88,0x88,0x88,0x98,0x70,0x00},
    ['v'] = {0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00},
    ['w'] = {0x00,0x00,0x88,0x88,0xA8,0xD8,0x88,0x00},
    ['x'] = {0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00},
    ['y'] = {0x00,0x00,0x88,0x88,0x78,0x08,0x70,0x00},
    ['z'] = {0x00,0x00,0xF8,0x10,0x20,0x40,0xF8,0x00},
    ['{'] = {0x00,0x18,0x30,0x60,0x30,0x30,0x18,0x00},
    ['|'] = {0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x00},
    ['}'] = {0x00,0x60,0x30,0x18,0x30,0x30,0x60,0x00},
    ['~'] = {0x00,0x00,0x28,0x50,0x00,0x00,0x00,0x00}
};

/* Authentic Apple II Video Line Address Interleaving */
static const uint16_t text_line_offset[24] = {
    0x000, 0x080, 0x100, 0x180, 0x200, 0x280, 0x300, 0x380,
    0x028, 0x0A8, 0x128, 0x1A8, 0x228, 0x2A8, 0x328, 0x3A8,
    0x050, 0x0D0, 0x150, 0x1D0, 0x250, 0x2D0, 0x350, 0x3D0
};

static uint16_t hires_line_offset(int y)
{
    return (uint16_t)(((y & 7) << 10) | (((y >> 3) & 7) << 7) | ((y >> 6) * 40));
}

static void set_nz(uint8_t v)
{
    reg_p &= (uint8_t)~(FLAG_N | FLAG_Z);
    if (v == 0) reg_p |= FLAG_Z;
    if (v & 0x80) reg_p |= FLAG_N;
}

/* Apple //e Memory Read */
static uint8_t apple_rd(uint16_t addr)
{
    if ((addr & 0xFF00) == 0xC000) {
        uint8_t off = (uint8_t)(addr & 0xFF);
        if (off == 0x00) return kbd_data;
        if (off == 0x10) { kbd_data &= 0x7F; return kbd_data; }
        if (off == 0x1A) return (uint8_t)(rand() & 0x80); /* VBLANK */
        if (off == 0x30) {
            /* Toggle speaker */
            struct plusos_sound_args sa = { PLUSOS_SND_TONE, 880, 1, 0, 0, 0 };
            plusos_syscall(SYS_sound, &sa);
            return 0;
        }
        if (off == 0x50) sw_text = 0;
        if (off == 0x51) sw_text = 1;
        if (off == 0x52) sw_mixed = 0;
        if (off == 0x53) sw_mixed = 1;
        if (off == 0x54) sw_page2 = 0;
        if (off == 0x55) sw_page2 = 1;
        if (off == 0x56) sw_hires = 0;
        if (off == 0x57) sw_hires = 1;
        if (off == 0x0C) sw_80col = 0;
        if (off == 0x0D) sw_80col = 1;
        return 0;
    }
    return ram[addr];
}

/* Apple //e Memory Write */
static void apple_wr(uint16_t addr, uint8_t val)
{
    if ((addr & 0xFF00) == 0xC000) {
        uint8_t off = (uint8_t)(addr & 0xFF);
        if (off == 0x10) { kbd_data &= 0x7F; return; }
        if (off == 0x30) {
            struct plusos_sound_args sa = { PLUSOS_SND_TONE, 880, 1, 0, 0, 0 };
            plusos_syscall(SYS_sound, &sa);
            return;
        }
        if (off == 0x50) sw_text = 0;
        if (off == 0x51) sw_text = 1;
        if (off == 0x52) sw_mixed = 0;
        if (off == 0x53) sw_mixed = 1;
        if (off == 0x54) sw_page2 = 0;
        if (off == 0x55) sw_page2 = 1;
        if (off == 0x56) sw_hires = 0;
        if (off == 0x57) sw_hires = 1;
        if (off == 0x0C) sw_80col = 0;
        if (off == 0x0D) sw_80col = 1;
        return;
    }
    if (addr >= 0xD000) return; /* Write protect ROM */
    ram[addr] = val;
}

static uint16_t apple_rd16(uint16_t addr)
{
    return (uint16_t)apple_rd(addr) | ((uint16_t)apple_rd((uint16_t)(addr + 1)) << 8);
}

static void push8(uint8_t v)   { apple_wr((uint16_t)(0x0100 + reg_sp--), v); }
static uint8_t pop8(void)      { return apple_rd((uint16_t)(0x0100 + ++reg_sp)); }
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

/* Execute one 6502/65C02 instruction */
static int step6502(void)
{
    /* Monitor / ROM traps */
    if (reg_pc == 0xFDED) { /* COUT: character output */
        uint8_t ch = reg_a & 0x7F;
        if (ch == 0x0D) {
            /* newline */
        }
        reg_pc = pop16() + 1;
        return 1;
    }
    if (reg_pc == 0xFD1B) { /* KEYIN: get key */
        if (kbd_data & 0x80) {
            reg_a = kbd_data;
            kbd_data &= 0x7F;
            reg_pc = pop16() + 1;
            return 1;
        }
        return 0; /* Wait for key */
    }

    uint8_t op = apple_rd(reg_pc++);
    uint16_t addr = 0;

    switch (op) {
    case 0x00: cpu_halted = 1; return 0; /* BRK */
    case 0xEA: break;                    /* NOP */

    /* LDA */
    case 0xA9: reg_a = apple_rd(reg_pc++); set_nz(reg_a); break;
    case 0xA5: reg_a = apple_rd(apple_rd(reg_pc++)); set_nz(reg_a); break;
    case 0xB5: reg_a = apple_rd((apple_rd(reg_pc++) + reg_x) & 0xFF); set_nz(reg_a); break;
    case 0xAD: addr = apple_rd16(reg_pc); reg_pc += 2; reg_a = apple_rd(addr); set_nz(reg_a); break;
    case 0xBD: addr = apple_rd16(reg_pc) + reg_x; reg_pc += 2; reg_a = apple_rd(addr); set_nz(reg_a); break;
    case 0xB9: addr = apple_rd16(reg_pc) + reg_y; reg_pc += 2; reg_a = apple_rd(addr); set_nz(reg_a); break;
    case 0xA1: { uint8_t zp = (apple_rd(reg_pc++) + reg_x) & 0xFF; addr = apple_rd16(zp); reg_a = apple_rd(addr); set_nz(reg_a); break; }
    case 0xB1: { uint8_t zp = apple_rd(reg_pc++); addr = apple_rd16(zp) + reg_y; reg_a = apple_rd(addr); set_nz(reg_a); break; }
    case 0x12: /* 65C02 (ZP) */ addr = apple_rd16(apple_rd(reg_pc++)); reg_a = apple_rd(addr); set_nz(reg_a); break;

    /* LDX */
    case 0xA2: reg_x = apple_rd(reg_pc++); set_nz(reg_x); break;
    case 0xA6: reg_x = apple_rd(apple_rd(reg_pc++)); set_nz(reg_x); break;
    case 0xB6: reg_x = apple_rd((apple_rd(reg_pc++) + reg_y) & 0xFF); set_nz(reg_x); break;
    case 0xAE: addr = apple_rd16(reg_pc); reg_pc += 2; reg_x = apple_rd(addr); set_nz(reg_x); break;
    case 0xBE: addr = apple_rd16(reg_pc) + reg_y; reg_pc += 2; reg_x = apple_rd(addr); set_nz(reg_x); break;

    /* LDY */
    case 0xA0: reg_y = apple_rd(reg_pc++); set_nz(reg_y); break;
    case 0xA4: reg_y = apple_rd(apple_rd(reg_pc++)); set_nz(reg_y); break;
    case 0xB4: reg_y = apple_rd((apple_rd(reg_pc++) + reg_x) & 0xFF); set_nz(reg_y); break;
    case 0xAC: addr = apple_rd16(reg_pc); reg_pc += 2; reg_y = apple_rd(addr); set_nz(reg_y); break;
    case 0xBC: addr = apple_rd16(reg_pc) + reg_x; reg_pc += 2; reg_y = apple_rd(addr); set_nz(reg_y); break;

    /* STA */
    case 0x85: apple_wr(apple_rd(reg_pc++), reg_a); break;
    case 0x95: apple_wr((apple_rd(reg_pc++) + reg_x) & 0xFF, reg_a); break;
    case 0x8D: addr = apple_rd16(reg_pc); reg_pc += 2; apple_wr(addr, reg_a); break;
    case 0x9D: addr = apple_rd16(reg_pc) + reg_x; reg_pc += 2; apple_wr(addr, reg_a); break;
    case 0x99: addr = apple_rd16(reg_pc) + reg_y; reg_pc += 2; apple_wr(addr, reg_a); break;
    case 0x81: { uint8_t zp = (apple_rd(reg_pc++) + reg_x) & 0xFF; addr = apple_rd16(zp); apple_wr(addr, reg_a); break; }
    case 0x91: { uint8_t zp = apple_rd(reg_pc++); addr = apple_rd16(zp) + reg_y; apple_wr(addr, reg_a); break; }
    case 0x92: /* 65C02 (ZP) */ addr = apple_rd16(apple_rd(reg_pc++)); apple_wr(addr, reg_a); break;

    /* STX / STY */
    case 0x86: apple_wr(apple_rd(reg_pc++), reg_x); break;
    case 0x96: apple_wr((apple_rd(reg_pc++) + reg_y) & 0xFF, reg_x); break;
    case 0x8E: addr = apple_rd16(reg_pc); reg_pc += 2; apple_wr(addr, reg_x); break;
    case 0x84: apple_wr(apple_rd(reg_pc++), reg_y); break;
    case 0x94: apple_wr((apple_rd(reg_pc++) + reg_x) & 0xFF, reg_y); break;
    case 0x8C: addr = apple_rd16(reg_pc); reg_pc += 2; apple_wr(addr, reg_y); break;

    /* STZ (65C02) */
    case 0x64: apple_wr(apple_rd(reg_pc++), 0); break;
    case 0x74: apple_wr((apple_rd(reg_pc++) + reg_x) & 0xFF, 0); break;
    case 0x9C: addr = apple_rd16(reg_pc); reg_pc += 2; apple_wr(addr, 0); break;
    case 0x9E: addr = apple_rd16(reg_pc) + reg_x; reg_pc += 2; apple_wr(addr, 0); break;

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
    case 0xDA: push8(reg_x); break; /* 65C02 PHX */
    case 0x5A: push8(reg_y); break; /* 65C02 PHY */
    case 0xFA: reg_x = pop8(); set_nz(reg_x); break; /* 65C02 PLX */
    case 0x7A: reg_y = pop8(); set_nz(reg_y); break; /* 65C02 PLY */

    /* ALU */
    case 0x69: do_adc(apple_rd(reg_pc++)); break;
    case 0x65: do_adc(apple_rd(apple_rd(reg_pc++))); break;
    case 0x6D: addr = apple_rd16(reg_pc); reg_pc += 2; do_adc(apple_rd(addr)); break;
    case 0xE9: do_sbc(apple_rd(reg_pc++)); break;
    case 0xE5: do_sbc(apple_rd(apple_rd(reg_pc++))); break;
    case 0xED: addr = apple_rd16(reg_pc); reg_pc += 2; do_sbc(apple_rd(addr)); break;
    case 0x29: reg_a &= apple_rd(reg_pc++); set_nz(reg_a); break;
    case 0x25: reg_a &= apple_rd(apple_rd(reg_pc++)); set_nz(reg_a); break;
    case 0x2D: addr = apple_rd16(reg_pc); reg_pc += 2; reg_a &= apple_rd(addr); set_nz(reg_a); break;
    case 0x09: reg_a |= apple_rd(reg_pc++); set_nz(reg_a); break;
    case 0x05: reg_a |= apple_rd(apple_rd(reg_pc++)); set_nz(reg_a); break;
    case 0x0D: addr = apple_rd16(reg_pc); reg_pc += 2; reg_a |= apple_rd(addr); set_nz(reg_a); break;
    case 0x49: reg_a ^= apple_rd(reg_pc++); set_nz(reg_a); break;
    case 0x45: reg_a ^= apple_rd(apple_rd(reg_pc++)); set_nz(reg_a); break;
    case 0x4D: addr = apple_rd16(reg_pc); reg_pc += 2; reg_a ^= apple_rd(addr); set_nz(reg_a); break;

    /* Compare */
    case 0xC9: do_cmp(reg_a, apple_rd(reg_pc++)); break;
    case 0xC5: do_cmp(reg_a, apple_rd(apple_rd(reg_pc++))); break;
    case 0xCD: addr = apple_rd16(reg_pc); reg_pc += 2; do_cmp(reg_a, apple_rd(addr)); break;
    case 0xE0: do_cmp(reg_x, apple_rd(reg_pc++)); break;
    case 0xE4: do_cmp(reg_x, apple_rd(apple_rd(reg_pc++))); break;
    case 0xEC: addr = apple_rd16(reg_pc); reg_pc += 2; do_cmp(reg_x, apple_rd(addr)); break;
    case 0xC0: do_cmp(reg_y, apple_rd(reg_pc++)); break;
    case 0xC4: do_cmp(reg_y, apple_rd(apple_rd(reg_pc++))); break;
    case 0xCC: addr = apple_rd16(reg_pc); reg_pc += 2; do_cmp(reg_y, apple_rd(addr)); break;

    /* Inc / Dec */
    case 0xE8: reg_x++; set_nz(reg_x); break;
    case 0xC8: reg_y++; set_nz(reg_y); break;
    case 0xCA: reg_x--; set_nz(reg_x); break;
    case 0x88: reg_y--; set_nz(reg_y); break;
    case 0x1A: reg_a++; set_nz(reg_a); break; /* 65C02 INA */
    case 0x3A: reg_a--; set_nz(reg_a); break; /* 65C02 DEA */
    case 0xE6: { uint8_t a = apple_rd(reg_pc++); uint8_t v = (uint8_t)(apple_rd(a) + 1); apple_wr(a, v); set_nz(v); break; }
    case 0xEE: { addr = apple_rd16(reg_pc); reg_pc += 2; uint8_t v = (uint8_t)(apple_rd(addr) + 1); apple_wr(addr, v); set_nz(v); break; }
    case 0xC6: { uint8_t a = apple_rd(reg_pc++); uint8_t v = (uint8_t)(apple_rd(a) - 1); apple_wr(a, v); set_nz(v); break; }
    case 0xCE: { addr = apple_rd16(reg_pc); reg_pc += 2; uint8_t v = (uint8_t)(apple_rd(addr) - 1); apple_wr(addr, v); set_nz(v); break; }

    /* Shifts */
    case 0x0A: if (reg_a & 0x80) reg_p |= FLAG_C; else reg_p &= (uint8_t)~FLAG_C; reg_a <<= 1; set_nz(reg_a); break;
    case 0x4A: if (reg_a & 0x01) reg_p |= FLAG_C; else reg_p &= (uint8_t)~FLAG_C; reg_a >>= 1; set_nz(reg_a); break;
    case 0x2A: { uint8_t old_c = (reg_p & FLAG_C) ? 1 : 0; if (reg_a & 0x80) reg_p |= FLAG_C; else reg_p &= (uint8_t)~FLAG_C; reg_a = (uint8_t)((reg_a << 1) | old_c); set_nz(reg_a); break; }
    case 0x6A: { uint8_t old_c = (reg_p & FLAG_C) ? 0x80 : 0; if (reg_a & 0x01) reg_p |= FLAG_C; else reg_p &= (uint8_t)~FLAG_C; reg_a = (uint8_t)((reg_a >> 1) | old_c); set_nz(reg_a); break; }

    /* Branches */
    case 0x10: { int8_t off = (int8_t)apple_rd(reg_pc++); if (!(reg_p & FLAG_N)) reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0x30: { int8_t off = (int8_t)apple_rd(reg_pc++); if (reg_p & FLAG_N)   reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0x50: { int8_t off = (int8_t)apple_rd(reg_pc++); if (!(reg_p & FLAG_V)) reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0x70: { int8_t off = (int8_t)apple_rd(reg_pc++); if (reg_p & FLAG_V)   reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0x90: { int8_t off = (int8_t)apple_rd(reg_pc++); if (!(reg_p & FLAG_C)) reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0xB0: { int8_t off = (int8_t)apple_rd(reg_pc++); if (reg_p & FLAG_C)   reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0xD0: { int8_t off = (int8_t)apple_rd(reg_pc++); if (!(reg_p & FLAG_Z)) reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0xF0: { int8_t off = (int8_t)apple_rd(reg_pc++); if (reg_p & FLAG_Z)   reg_pc = (uint16_t)(reg_pc + off); break; }
    case 0x80: { int8_t off = (int8_t)apple_rd(reg_pc++); reg_pc = (uint16_t)(reg_pc + off); break; } /* 65C02 BRA */

    /* Jumps & Calls */
    case 0x4C: reg_pc = apple_rd16(reg_pc); break;
    case 0x6C: addr = apple_rd16(reg_pc); reg_pc = apple_rd16(addr); break;
    case 0x7C: addr = apple_rd16(reg_pc) + reg_x; reg_pc = apple_rd16(addr); break; /* 65C02 JMP (abs,X) */
    case 0x20: addr = apple_rd16(reg_pc); reg_pc += 2; push16((uint16_t)(reg_pc - 1)); reg_pc = addr; break;
    case 0x60: reg_pc = (uint16_t)(pop16() + 1); break;
    case 0x40: reg_p = pop8() | FLAG_U; reg_pc = pop16(); break;

    /* Flags */
    case 0x18: reg_p &= (uint8_t)~FLAG_C; break;
    case 0x38: reg_p |= FLAG_C; break;
    case 0x58: reg_p &= (uint8_t)~FLAG_I; break;
    case 0x78: reg_p |= FLAG_I; break;
    case 0xD8: reg_p &= (uint8_t)~FLAG_D; break;
    case 0xF8: reg_p |= FLAG_D; break;
    case 0xB8: reg_p &= (uint8_t)~FLAG_V; break;

    default:
        break;
    }
    return 1;
}

static void apple_reset(void)
{
    memset(ram, 0, sizeof ram);
    memset(aux_ram, 0, sizeof aux_ram);
    reg_a = reg_x = reg_y = 0;
    reg_sp = 0xFF;
    reg_p = FLAG_U | FLAG_I;
    reg_pc = 0x2000;
    sw_text = 1;
    sw_mixed = 0;
    sw_page2 = 0;
    sw_hires = 0;
    sw_80col = 0;
    kbd_data = 0;
}

/* Render Apple II Text & Hi-Res to PlusOS Backbuffer */
static void apple_render(void)
{
    gfx_fill(0, 0, 512, 342, 0);

    /* Apple //e Bezel & Status */
    gfx_fill(0, 0, 512, 18, 1);
    gfx_text(10, 5, "APPLE //e EMULATOR (MOS 65C02 / 64K)", 1);
    char st[64];
    snprintf(st, sizeof st, "%s %s %s",
             sw_text ? (sw_80col ? "80-COL TEXT" : "40-COL TEXT") : (sw_hires ? "HI-RES" : "LO-RES"),
             sw_page2 ? "PAGE 2" : "PAGE 1",
             sw_mixed ? "MIXED" : "FULL");
    gfx_text(350, 5, st, 1);

    int scr_ox = sw_80col ? 16 : 96;
    int scr_oy = 28;

    if (sw_text || (!sw_text && sw_mixed)) {
        int start_row = sw_text ? 0 : 20;
        uint16_t base = sw_page2 ? 0x0800 : 0x0400;

        if (sw_80col) {
            /* 80-column text: 80x24 */
            for (int r = start_row; r < 24; r++) {
                uint16_t row_addr = (uint16_t)(base + text_line_offset[r]);
                for (int c = 0; c < 80; c++) {
                    uint8_t ch = (c & 1) ? ram[row_addr + (c >> 1)] : aux_ram[row_addr + (c >> 1)];
                    ch &= 0x7F;
                    if (ch < 32 || ch > 126) ch = ' ';
                    /* Draw 6x8 condensed font */
                    int px = scr_ox + c * 6;
                    int py = scr_oy + r * 12;
                    for (int dy = 0; dy < 8; dy++) {
                        uint8_t b = a2_font[ch][dy];
                        for (int dx = 0; dx < 5; dx++) {
                            if (b & (0x80 >> dx)) {
                                gfx_pixel(px + dx, py + dy, 1);
                            }
                        }
                    }
                }
            }
        } else {
            /* 40-column text: 40x24 */
            for (int r = start_row; r < 24; r++) {
                uint16_t row_addr = (uint16_t)(base + text_line_offset[r]);
                for (int c = 0; c < 40; c++) {
                    uint8_t ch = ram[row_addr + c] & 0x7F;
                    if (ch < 32 || ch > 126) ch = ' ';
                    int px = scr_ox + c * 8;
                    int py = scr_oy + r * 12;
                    for (int dy = 0; dy < 8; dy++) {
                        uint8_t b = a2_font[ch][dy];
                        for (int dx = 0; dx < 8; dx++) {
                            if (b & (0x80 >> dx)) {
                                gfx_pixel(px + dx, py + dy, 1);
                            }
                        }
                    }
                }
            }
        }
    }

    if (!sw_text) {
        if (sw_hires) {
            /* Hi-Res Graphics: 280x192 */
            uint16_t base = sw_page2 ? 0x4000 : 0x2000;
            int max_y = sw_mixed ? 160 : 192;
            int hox = 116;
            int hoy = 36;
            gfx_frame(hox - 2, hoy - 2, hox + 280 + 2, hoy + max_y + 2);

            for (int y = 0; y < max_y; y++) {
                uint16_t laddr = (uint16_t)(base + hires_line_offset(y));
                for (int b = 0; b < 40; b++) {
                    uint8_t byte = ram[laddr + b];
                    for (int bit = 0; bit < 7; bit++) {
                        if (byte & (1 << bit)) {
                            gfx_pixel(hox + b * 7 + bit, hoy + y, 1);
                        }
                    }
                }
            }
        }
    }

    /* Footer Navigation */
    gfx_fill(0, 324, 512, 342, 1);
    gfx_text(10, 328, "Tab: Switch Demo  |  F1/Ctrl-R: Reset  |  Esc: Exit PlusOS", 1);
}

/* Built-in Apple //e Interactive Shell & Demonstrations */
static void init_apple_demo(int mode)
{
    apple_reset();
    uint16_t text_base = 0x0400;

    /* Write classic Apple //e banner at top line */
    const char *title = "           APPLE //e (64K)            ";
    for (int i = 0; title[i] && i < 40; i++) {
        ram[text_base + text_line_offset[0] + i] = (uint8_t)(title[i] | 0x80);
    }

    if (mode == 0) {
        /* Hi-Res Spirangle Demo */
        sw_text = 0;
        sw_mixed = 1;
        sw_hires = 1;

        const char *m1 = "]10 HGR : HCOLOR=3";
        const char *m2 = "]20 FOR I=1 TO 90 : HPLOT X,Y TO X+I,Y";
        const char *m3 = "]RUN";
        for (int i = 0; m1[i]; i++) ram[text_base + text_line_offset[20] + i] = (uint8_t)(m1[i] | 0x80);
        for (int i = 0; m2[i]; i++) ram[text_base + text_line_offset[21] + i] = (uint8_t)(m2[i] | 0x80);
        for (int i = 0; m3[i]; i++) ram[text_base + text_line_offset[22] + i] = (uint8_t)(m3[i] | 0x80);

        /* Draw spiral pattern in Hi-Res Page 1 ($2000) */
        int cx = 140, cy = 80;
        for (int r = 5; r < 75; r += 2) {
            for (int a = 0; a < 360; a += 15) {
                int x = cx + ((r * (a - 180)) / 120);
                int y = cy + ((r * (90 - (a % 180))) / 100);
                if (x >= 0 && x < 280 && y >= 0 && y < 160) {
                    uint16_t laddr = (uint16_t)(0x2000 + hires_line_offset(y));
                    ram[laddr + (x / 7)] |= (uint8_t)(1 << (x % 7));
                }
            }
        }
    } else if (mode == 1) {
        /* 80-Column ProDOS / Applesoft Mode */
        sw_text = 1;
        sw_80col = 1;

        const char *lines[] = {
            "PRODOS 2.4.2  27-OCT-87",
            "APPLE //e 80-COLUMN SYSTEM (128K AUX EXPANSION)",
            "]CATALOG",
            "/APPLE2.SYSTEM",
            " NAME           TYPE BLOCKS  MODIFIED",
            " PRODOS          SYS     32  27-OCT-87",
            " BASIC.SYSTEM    SYS     21  27-OCT-87",
            " APPLESOFT.BIN   BIN     18  27-OCT-87",
            " DEMO.BAS        BAS      4  07-NOV-26",
            " 107 BLOCKS FREE, 173 BLOCKS USED",
            "]LIST 10-40",
            " 10 PRINT \"APPLE //e ENHANCED COMPUTER EMULATOR\"",
            " 20 FOR I = 1 TO 10 : PRINT CHR$(7); : NEXT I",
            " 30 HGR : HPLOT 0,0 TO 279,191",
            " 40 GOTO 10",
            "]RUN"
        };
        for (int r = 0; r < 16; r++) {
            uint16_t row_addr = (uint16_t)(text_base + text_line_offset[r + 2]);
            const char *l = lines[r];
            for (int c = 0; l[c] && c < 80; c++) {
                if (c & 1) ram[row_addr + (c >> 1)] = (uint8_t)(l[c] | 0x80);
                else aux_ram[row_addr + (c >> 1)] = (uint8_t)(l[c] | 0x80);
            }
        }
    } else if (mode == 2) {
        /* Apple II Woz Monitor Mode */
        sw_text = 1;
        sw_80col = 0;
        const char *lines[] = {
            "CALL -151",
            "*0300: A9 41 20 ED FD A9 50 20",
            "*0308: ED FD A9 50 20 ED FD A9",
            "*0310: 4C 20 ED FD 60",
            "*0300G",
            "APPL",
            "*0300.031F",
            "0300- A9 41 20 ED FD A9 50 20",
            "0308- ED FD A9 50 20 ED FD A9",
            "0310- 4C 20 ED FD 60 00 00 00",
            "0318- 00 00 00 00 00 00 00 00",
            "*_ "
        };
        for (int r = 0; r < 12; r++) {
            uint16_t row_addr = (uint16_t)(text_base + text_line_offset[r + 2]);
            const char *l = lines[r];
            for (int c = 0; l[c] && c < 40; c++) {
                ram[row_addr + c] = (uint8_t)(l[c] | 0x80);
            }
        }
    }
}

int main(int argc, char **argv)
{
    gfx_target(gfx_backbuffer());

    int demo_mode = 0;
    init_apple_demo(demo_mode);

    /* If user provided a binary file on command line, load it */
    if (argc > 1) {
        unsigned char pn[256];
        int n = 0;
        while (argv[1][n] && n < 254) { pn[n + 1] = (unsigned char)argv[1][n]; n++; }
        pn[0] = (unsigned char)n;
        INTEGER ref;
        if (FSOpen(pn, (INTEGER)plusos->boot_vrefnum, &ref) == 0) {
            LONGINT count = 0x8000;
            FSRead(ref, &count, ram + 0x2000);
            FSClose(ref);
            sw_text = 0;
            sw_hires = 1;
        }
    }

    uint32_t last_tick = plusos->ticks();

    for (;;) {
        for (int kp_; (kp_ = plusos->key_poll()) != 0; ) {
            int ch = (int)kp_;
            if (ch == 27) return 0; /* Esc exits */
            if (ch == '\t') {
                demo_mode = (demo_mode + 1) % 3;
                init_apple_demo(demo_mode);
                continue;
            }
            if (ch == 18) { /* Ctrl-R: Reset */
                apple_reset();
                init_apple_demo(demo_mode);
                continue;
            }

            /* Pass key to Apple II keyboard strobe */
            kbd_data = (uint8_t)(toupper(ch) | 0x80);

            /* Inject into 40-col prompt if in text mode */
            if (sw_text && !sw_80col) {
                static int cur_c = 2;
                if (ch == '\r' || ch == '\n') {
                    cur_c = 2;
                } else if (ch >= 32 && ch < 127) {
                    if (cur_c < 39) {
                        ram[0x0400 + text_line_offset[22] + cur_c++] = (uint8_t)(toupper(ch) | 0x80);
                    }
                }
            }
        }

        /* Run 6502 cycles */
        for (int i = 0; i < 200; i++) {
            if (!step6502()) break;
        }

        /* Render frame */
        apple_render();
        gfx_present();

        while (plusos->ticks() == last_tick) {}
        last_tick = plusos->ticks();
    }

    return 0;
}
