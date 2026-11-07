/* emu6502.c - MOS 6502 CPU & Apple I Woz Monitor Emulator for PlusOS
 *
 * Emulates a 64 KB MOS 6502 computer with Apple I PIA I/O ($D010..$D013) and
 * built-in 6502 machine-code programs:
 *   $0200: 6502 Welcome Banner (JSR $FFEF character output)
 *   $0300: 6502 Fibonacci Hex Sequence Generator
 *   $0400: 6502 ASCII Sierpinski Triangle Pattern
 *
 * Interactive monitor commands:
 *   200R / 300R / 400R   Run 6502 machine code at hex address
 *   200.22F              Hex dump 6502 memory range
 *   200: A9 41 20 EF FF  Deposit bytes into 6502 RAM
 *   regs                 Display 6502 registers (A, X, Y, SP, PC, P)
 *   step [N]             Single-step N instructions with trace
 *   help                 Show commands
 *   quit                 Exit emulator
 */
#include <plusos/plusos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static uint8_t  mem6502[65536];
static uint8_t  reg_a, reg_x, reg_y, reg_sp, reg_p;
static uint16_t reg_pc;
static int      cpu_halted;

enum {
    FLAG_C = 0x01, FLAG_Z = 0x02, FLAG_I = 0x04, FLAG_D = 0x08,
    FLAG_B = 0x10, FLAG_U = 0x20, FLAG_V = 0x40, FLAG_N = 0x80
};

static void set_nz(uint8_t v)
{
    reg_p &= ~(FLAG_N | FLAG_Z);
    if (v == 0) reg_p |= FLAG_Z;
    if (v & 0x80) reg_p |= FLAG_N;
}

static uint8_t rd(uint16_t addr)
{
    if (addr == 0xD010) {
        int k = plusos->key_poll();
        if (k) { k = plusos->key_get(); return (uint8_t)(toupper(k) | 0x80); }
        return 0;
    }
    if (addr == 0xD011) {
        return plusos->key_poll() ? 0x80 : 0x00;
    }
    return mem6502[addr];
}

static void wr(uint16_t addr, uint8_t val)
{
    if (addr == 0xD012) {
        char c = (char)(val & 0x7F);
        if (c == '\r') putchar('\n');
        else if (c >= 0x20 && c < 0x7F) putchar(c);
        return;
    }
    if (addr >= 0xFF00) return; /* ROM area */
    mem6502[addr] = val;
}

static uint16_t rd16(uint16_t addr)
{
    return (uint16_t)rd(addr) | ((uint16_t)rd((uint16_t)(addr + 1)) << 8);
}

static void push8(uint8_t v) { wr((uint16_t)(0x0100 + reg_sp--), v); }
static uint8_t pop8(void)    { return rd((uint16_t)(0x0100 + ++reg_sp)); }
static void push16(uint16_t v) { push8((uint8_t)(v >> 8)); push8((uint8_t)v); }
static uint16_t pop16(void)  { uint8_t lo = pop8(); return lo | ((uint16_t)pop8() << 8); }

static void do_adc(uint8_t m)
{
    uint16_t sum = (uint16_t)reg_a + m + (reg_p & FLAG_C ? 1 : 0);
    if (sum > 0xFF) reg_p |= FLAG_C; else reg_p &= ~FLAG_C;
    if (~(reg_a ^ m) & (reg_a ^ sum) & 0x80) reg_p |= FLAG_V; else reg_p &= ~FLAG_V;
    reg_a = (uint8_t)sum;
    set_nz(reg_a);
}

static void do_sbc(uint8_t m) { do_adc((uint8_t)~m); }

static void do_cmp(uint8_t r, uint8_t m)
{
    uint16_t d = (uint16_t)r - m;
    if (r >= m) reg_p |= FLAG_C; else reg_p &= ~FLAG_C;
    set_nz((uint8_t)d);
}

/* Execute one 6502 instruction; returns 0 on BRK/RTS-to-monitor */
static int step6502(void)
{
    /* Intercept Apple I Woz Monitor ROM subroutines:
     *   $FFEF (ECHO): output char in A
     *   $FFDC (PRBYTE): output A as 2 hex digits
     */
    if (reg_pc == 0xFFEF) {
        wr(0xD012, reg_a);
        reg_pc = pop16() + 1;
        return 1;
    }
    if (reg_pc == 0xFFDC) {
        printf("%02X", reg_a);
        reg_pc = pop16() + 1;
        return 1;
    }
    if (reg_pc == 0xFF00) {
        cpu_halted = 1;
        return 0;
    }

    uint8_t op = rd(reg_pc++);
    uint16_t addr = 0;

    switch (op) {
    case 0x00: cpu_halted = 1; return 0; /* BRK returns to monitor */
    case 0xEA: break;                    /* NOP */

    /* LDA */
    case 0xA9: reg_a = rd(reg_pc++); set_nz(reg_a); break;
    case 0xA5: reg_a = rd(rd(reg_pc++)); set_nz(reg_a); break;
    case 0xB5: reg_a = rd((rd(reg_pc++) + reg_x) & 0xFF); set_nz(reg_a); break;
    case 0xAD: addr = rd16(reg_pc); reg_pc += 2; reg_a = rd(addr); set_nz(reg_a); break;
    case 0xBD: addr = rd16(reg_pc) + reg_x; reg_pc += 2; reg_a = rd(addr); set_nz(reg_a); break;
    case 0xB9: addr = rd16(reg_pc) + reg_y; reg_pc += 2; reg_a = rd(addr); set_nz(reg_a); break;
    case 0xB1: addr = rd16(rd(reg_pc++)) + reg_y; reg_a = rd(addr); set_nz(reg_a); break;

    /* LDX */
    case 0xA2: reg_x = rd(reg_pc++); set_nz(reg_x); break;
    case 0xA6: reg_x = rd(rd(reg_pc++)); set_nz(reg_x); break;
    case 0xAE: addr = rd16(reg_pc); reg_pc += 2; reg_x = rd(addr); set_nz(reg_x); break;

    /* LDY */
    case 0xA0: reg_y = rd(reg_pc++); set_nz(reg_y); break;
    case 0xA4: reg_y = rd(rd(reg_pc++)); set_nz(reg_y); break;
    case 0xAC: addr = rd16(reg_pc); reg_pc += 2; reg_y = rd(addr); set_nz(reg_y); break;

    /* STA */
    case 0x85: wr(rd(reg_pc++), reg_a); break;
    case 0x95: wr((rd(reg_pc++) + reg_x) & 0xFF, reg_a); break;
    case 0x8D: addr = rd16(reg_pc); reg_pc += 2; wr(addr, reg_a); break;
    case 0x9D: addr = rd16(reg_pc) + reg_x; reg_pc += 2; wr(addr, reg_a); break;
    case 0x99: addr = rd16(reg_pc) + reg_y; reg_pc += 2; wr(addr, reg_a); break;
    case 0x91: addr = rd16(rd(reg_pc++)) + reg_y; wr(addr, reg_a); break;

    /* STX / STY */
    case 0x86: wr(rd(reg_pc++), reg_x); break;
    case 0x8E: addr = rd16(reg_pc); reg_pc += 2; wr(addr, reg_x); break;
    case 0x84: wr(rd(reg_pc++), reg_y); break;
    case 0x8C: addr = rd16(reg_pc); reg_pc += 2; wr(addr, reg_y); break;

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

    /* ALU Immediate & ZP */
    case 0x69: do_adc(rd(reg_pc++)); break;
    case 0x65: do_adc(rd(rd(reg_pc++))); break;
    case 0xE9: do_sbc(rd(reg_pc++)); break;
    case 0xE5: do_sbc(rd(rd(reg_pc++))); break;
    case 0x29: reg_a &= rd(reg_pc++); set_nz(reg_a); break;
    case 0x25: reg_a &= rd(rd(reg_pc++)); set_nz(reg_a); break;
    case 0x09: reg_a |= rd(reg_pc++); set_nz(reg_a); break;
    case 0x05: reg_a |= rd(rd(reg_pc++)); set_nz(reg_a); break;
    case 0x49: reg_a ^= rd(reg_pc++); set_nz(reg_a); break;
    case 0x45: reg_a ^= rd(rd(reg_pc++)); set_nz(reg_a); break;

    /* Compare */
    case 0xC9: do_cmp(reg_a, rd(reg_pc++)); break;
    case 0xC5: do_cmp(reg_a, rd(rd(reg_pc++))); break;
    case 0xE0: do_cmp(reg_x, rd(reg_pc++)); break;
    case 0xC0: do_cmp(reg_y, rd(reg_pc++)); break;

    /* Inc / Dec */
    case 0xE8: reg_x++; set_nz(reg_x); break;
    case 0xC8: reg_y++; set_nz(reg_y); break;
    case 0xCA: reg_x--; set_nz(reg_x); break;
    case 0x88: reg_y--; set_nz(reg_y); break;
    case 0xE6: { uint8_t a = rd(reg_pc++); uint8_t v = (uint8_t)(rd(a) + 1); wr(a, v); set_nz(v); break; }
    case 0xC6: { uint8_t a = rd(reg_pc++); uint8_t v = (uint8_t)(rd(a) - 1); wr(a, v); set_nz(v); break; }

    /* Shifts */
    case 0x0A: if (reg_a & 0x80) reg_p |= FLAG_C; else reg_p &= ~FLAG_C; reg_a <<= 1; set_nz(reg_a); break;
    case 0x4A: if (reg_a & 0x01) reg_p |= FLAG_C; else reg_p &= ~FLAG_C; reg_a >>= 1; set_nz(reg_a); break;

    /* Branches */
    case 0x90: { int8_t rel = (int8_t)rd(reg_pc++); if (!(reg_p & FLAG_C)) reg_pc = (uint16_t)(reg_pc + rel); break; }
    case 0xB0: { int8_t rel = (int8_t)rd(reg_pc++); if (reg_p & FLAG_C)    reg_pc = (uint16_t)(reg_pc + rel); break; }
    case 0xF0: { int8_t rel = (int8_t)rd(reg_pc++); if (reg_p & FLAG_Z)    reg_pc = (uint16_t)(reg_pc + rel); break; }
    case 0xD0: { int8_t rel = (int8_t)rd(reg_pc++); if (!(reg_p & FLAG_Z)) reg_pc = (uint16_t)(reg_pc + rel); break; }
    case 0x30: { int8_t rel = (int8_t)rd(reg_pc++); if (reg_p & FLAG_N)    reg_pc = (uint16_t)(reg_pc + rel); break; }
    case 0x10: { int8_t rel = (int8_t)rd(reg_pc++); if (!(reg_p & FLAG_N)) reg_pc = (uint16_t)(reg_pc + rel); break; }

    /* Jumps & Subroutines */
    case 0x4C: reg_pc = rd16(reg_pc); break;
    case 0x20: addr = rd16(reg_pc); reg_pc += 2; push16((uint16_t)(reg_pc - 1)); reg_pc = addr; break;
    case 0x60: reg_pc = pop16() + 1; break;

    /* Flags */
    case 0x18: reg_p &= ~FLAG_C; break;
    case 0x38: reg_p |= FLAG_C; break;
    case 0xD8: reg_p &= ~FLAG_D; break;
    case 0xF8: reg_p |= FLAG_D; break;
    case 0x58: reg_p &= ~FLAG_I; break;
    case 0x78: reg_p |= FLAG_I; break;
    case 0xB8: reg_p &= ~FLAG_V; break;
    default: break;
    }
    return 1;
}

static void seed_programs(void)
{
    memset(mem6502, 0, sizeof mem6502);
    reg_a = reg_x = reg_y = 0;
    reg_sp = 0xFD;
    reg_p = FLAG_U | FLAG_I;
    reg_pc = 0x0200;

    /* $0200: 6502 Hello World Banner
     *   LDX #$00
     * loop: LDA $0210,X ; BEQ done ; JSR $FFEF ; INX ; BNE loop
     * done: BRK
     */
    static const uint8_t prog_hello[] = {
        0xA2, 0x00,
        0xBD, 0x10, 0x02,
        0xF0, 0x07,
        0x20, 0xEF, 0xFF,
        0xE8,
        0xD0, 0xF5,
        0x00, 0x00, 0x00,
        'H','E','L','L','O',' ','F','R','O','M',' ','M','O','S',' ','6','5','0','2',' ',
        'O','N',' ','P','L','U','S','O','S','!','\r', 0
    };
    memcpy(mem6502 + 0x0200, prog_hello, sizeof prog_hello);

    /* $0300: 6502 Fibonacci Sequence (prints 12 hex terms via JSR $FFDC) */
    static const uint8_t prog_fib[] = {
        0xA9, 0x00, 0x85, 0x10,             /* LDA #0; STA $10 */
        0xA9, 0x01, 0x85, 0x11,             /* LDA #1; STA $11 */
        0xA2, 0x0C,                         /* LDX #12 */
        /* loop ($030A): */
        0xA5, 0x11, 0x20, 0xDC, 0xFF,       /* LDA $11; JSR $FFDC (PRBYTE) */
        0xA9, 0x20, 0x20, 0xEF, 0xFF,       /* LDA #' '; JSR $FFEF (ECHO) */
        0x18, 0xA5, 0x10, 0x65, 0x11,       /* CLC; LDA $10; ADC $11 */
        0xA4, 0x11, 0x84, 0x10, 0x85, 0x11, /* LDY $11; STY $10; STA $11 */
        0xCA, 0xD0, 0xEA,                   /* DEX; BNE loop */
        0xA9, 0x0D, 0x20, 0xEF, 0xFF,       /* LDA #CR; JSR $FFEF */
        0x00                                /* BRK */
    };
    memcpy(mem6502 + 0x0300, prog_fib, sizeof prog_fib);

    /* $0400: 6502 Rule-90 / Sierpinski Bitwise Triangle */
    static const uint8_t prog_sierp[] = {
        0xA0, 0x00,                         /* LDY #0 (row = 0..15) */
        /* row_loop ($0402): */
        0xA2, 0x00,                         /* LDX #0 (col = 0..15) */
        /* col_loop ($0404): */
        0x8A, 0x85, 0x20,                   /* TXA; STA $20 */
        0x98, 0x25, 0x20,                   /* TYA; AND $20 */
        0xD0, 0x04,                         /* BNE space */
        0xA9, '*', 0xD0, 0x02,              /* LDA #'*' */
        0xA9, ' ',                          /* space: LDA #' ' */
        0x20, 0xEF, 0xFF,                   /* JSR $FFEF */
        0xE8, 0xE0, 0x10, 0xD0, 0xEC,       /* INX; CPX #16; BNE col_loop */
        0xA9, 0x0D, 0x20, 0xEF, 0xFF,       /* LDA #CR; JSR $FFEF */
        0xC8, 0xC0, 0x10, 0xD0, 0xE0,       /* INY; CPY #16; BNE row_loop */
        0x00                                /* BRK */
    };
    memcpy(mem6502 + 0x0400, prog_sierp, sizeof prog_sierp);
}

static void print_regs(void)
{
    printf("  PC=%04X  A=%02X  X=%02X  Y=%02X  SP=%02X  P=%02X [%c%c-%c%c%c%c%c]\n",
           reg_pc, reg_a, reg_x, reg_y, reg_sp, reg_p,
           (reg_p & FLAG_N) ? 'N' : '.', (reg_p & FLAG_V) ? 'V' : '.',
           (reg_p & FLAG_B) ? 'B' : '.', (reg_p & FLAG_D) ? 'D' : '.',
           (reg_p & FLAG_I) ? 'I' : '.', (reg_p & FLAG_Z) ? 'Z' : '.',
           (reg_p & FLAG_C) ? 'C' : '.');
}

static void run_from(uint16_t start_pc)
{
    reg_pc = start_pc;
    cpu_halted = 0;
    push16(0xFF00 - 1); /* RTS returns to monitor ($FF00) */
    int steps = 0;
    while (!cpu_halted && steps < 200000) {
        if (!step6502()) break;
        steps++;
    }
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    seed_programs();

    printf("MOS 6502 CPU & Apple I Woz Monitor Emulator for PlusOS\n");
    printf("Built-in 6502 machine-code programs:\n");
    printf("  200R       Run Hello World ($0200)\n");
    printf("  300R       Run Fibonacci Hex Generator ($0300)\n");
    printf("  400R       Run Sierpinski Triangle ($0400)\n");
    printf("  200.22F    Hex dump memory   |  200: A9 42 00  Write bytes\n");
    printf("  regs       Show CPU state    |  step [N]       Single-step\n");
    printf("  quit       Return to PlusOS\n\n");

    char line[128];
    uint16_t last_addr = 0x0200;

    for (;;) {
        printf("6502> ");
        if (plusos->key_line(line, sizeof line) <= 0) continue;
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) continue;

        if (!strcasecmp(p, "quit") || !strcasecmp(p, "exit") || !strcasecmp(p, "q"))
            break;
        if (!strcasecmp(p, "help")) {
            printf("  <HEX>R           Execute 6502 code at <HEX> (e.g. 200R, 300R, 400R)\n");
            printf("  <START>.<END>    Examine memory range (e.g. 200.22F)\n");
            printf("  <ADDR>: <BYTE>.. Deposit hex bytes into 6502 RAM\n");
            printf("  regs             Print 6502 registers\n");
            printf("  step [N]         Single-step N instructions with register trace\n");
            printf("  reset            Reset 6502 CPU and reload built-in ROMs\n");
            continue;
        }
        if (!strcasecmp(p, "regs")) { print_regs(); continue; }
        if (!strcasecmp(p, "reset")) { seed_programs(); printf("6502 CPU and RAM reset.\n"); continue; }
        if (!strncasecmp(p, "step", 4)) {
            int n = atoi(p + 4);
            if (n <= 0) n = 1;
            for (int i = 0; i < n; i++) {
                printf("  [%04X: %02X] ", reg_pc, mem6502[reg_pc]);
                step6502();
                print_regs();
            }
            continue;
        }

        /* Woz Monitor syntax: ADDR R, START.END, ADDR: B1 B2 ... */
        char *endp;
        unsigned long a1 = strtoul(p, &endp, 16);
        if (endp != p) {
            last_addr = (uint16_t)a1;
            while (*endp == ' ') endp++;
            if (*endp == 'r' || *endp == 'R') {
                run_from(last_addr);
                continue;
            }
            if (*endp == '.') {
                uint16_t a2 = (uint16_t)strtoul(endp + 1, 0, 16);
                for (uint32_t a = last_addr; a <= a2; a += 8) {
                    printf("%04X: ", (unsigned)a);
                    for (uint32_t j = 0; j < 8 && a + j <= a2; j++)
                        printf("%02X ", mem6502[(a + j) & 0xFFFF]);
                    printf("\n");
                }
                continue;
            }
            if (*endp == ':') {
                char *bp = endp + 1;
                uint16_t cur = last_addr;
                for (;;) {
                    while (*bp == ' ') bp++;
                    if (!*bp) break;
                    char *next;
                    unsigned long b = strtoul(bp, &next, 16);
                    if (next == bp) break;
                    mem6502[cur++] = (uint8_t)b;
                    bp = next;
                }
                printf("Wrote %u bytes at $%04X.\n", (unsigned)(cur - last_addr), last_addr);
                continue;
            }
            printf("%04X: %02X\n", last_addr, mem6502[last_addr]);
            continue;
        }
        printf("Unknown command '%s' (try 200R, 300R, 400R, regs, help, quit)\n", p);
    }
    return 0;
}
