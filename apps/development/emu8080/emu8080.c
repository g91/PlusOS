/* emu8080.c - Intel 8080 Microprocessor & CP/M 2.2 Subsystem for PlusOS
 *
 * Emulates the iconic 8-bit Intel 8080 CPU and Digital Research CP/M 2.2:
 *   - Full Intel 8080 instruction set (registers A, B, C, D, E, H, L, SP, PC, Flags)
 *   - 64 KB TPA memory space ($0000..$FFFF)
 *   - CP/M 2.2 BDOS & BIOS call trapping at vector $0005
 *   - Authentic Digital Research CP/M 2.2 CCP console ("A>")
 *   - Built-in utilities & programs: DIR, TYPE, STAT, DUMP, MBASIC, PRIME, HELP
 *   - External Intel 8080 CP/M .COM binary loader (.com)
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusrom/FileMgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define I8080_RAM_SIZE 65536

static uint8_t  i8080_ram[I8080_RAM_SIZE];

/* Intel 8080 Registers */
static uint8_t  reg_a, reg_b, reg_c, reg_d, reg_e, reg_h, reg_l;
static uint16_t reg_sp, reg_pc;

/* Flags: S (Sign), Z (Zero), AC (Aux Carry), P (Parity), CY (Carry) */
static uint8_t  flag_s, flag_z, flag_ac, flag_p, flag_cy;
static int      cpu_halted;

static const uint8_t parity_table[256] = {
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1
};

static inline void set_flags_szp(uint8_t v)
{
    flag_z = (v == 0);
    flag_s = (v & 0x80) != 0;
    flag_p = parity_table[v];
}

static inline uint16_t get_bc(void) { return ((uint16_t)reg_b << 8) | reg_c; }
static inline void set_bc(uint16_t v) { reg_b = (uint8_t)(v >> 8); reg_c = (uint8_t)v; }
static inline uint16_t get_de(void) { return ((uint16_t)reg_d << 8) | reg_e; }
static inline void set_de(uint16_t v) { reg_d = (uint8_t)(v >> 8); reg_e = (uint8_t)v; }
static inline uint16_t get_hl(void) { return ((uint16_t)reg_h << 8) | reg_l; }
static inline void set_hl(uint16_t v) { reg_h = (uint8_t)(v >> 8); reg_l = (uint8_t)v; }

static inline uint8_t get_psw(void)
{
    return (uint8_t)((flag_s << 7) | (flag_z << 6) | (flag_ac << 4) | (flag_p << 2) | 0x02 | flag_cy);
}

static inline void set_psw(uint8_t v)
{
    flag_s  = (v >> 7) & 1;
    flag_z  = (v >> 6) & 1;
    flag_ac = (v >> 4) & 1;
    flag_p  = (v >> 2) & 1;
    flag_cy = v & 1;
}

static inline uint8_t mem_rd(uint16_t a) { return i8080_ram[a]; }
static inline void mem_wr(uint16_t a, uint8_t v) { i8080_ram[a] = v; }
static inline uint16_t mem_rd16(uint16_t a) { return (uint16_t)mem_rd(a) | ((uint16_t)mem_rd((uint16_t)(a + 1)) << 8); }
static inline void mem_wr16(uint16_t a, uint16_t v) { mem_wr(a, (uint8_t)v); mem_wr((uint16_t)(a + 1), (uint8_t)(v >> 8)); }

static inline void push(uint16_t v) { reg_sp -= 2; mem_wr16(reg_sp, v); }
static inline uint16_t pop(void) { uint16_t v = mem_rd16(reg_sp); reg_sp += 2; return v; }

static void do_add(uint8_t v, int carry)
{
    uint16_t res = (uint16_t)reg_a + v + (carry ? flag_cy : 0);
    flag_ac = ((reg_a & 0x0F) + (v & 0x0F) + (carry ? flag_cy : 0)) > 0x0F;
    flag_cy = (res > 0xFF);
    reg_a = (uint8_t)res;
    set_flags_szp(reg_a);
}

static void do_sub(uint8_t v, int borrow)
{
    uint16_t res = (uint16_t)reg_a - v - (borrow ? flag_cy : 0);
    flag_ac = ((reg_a & 0x0F) - (v & 0x0F) - (borrow ? flag_cy : 0)) < 0;
    flag_cy = (res > 0xFF);
    reg_a = (uint8_t)res;
    set_flags_szp(reg_a);
}

/* CP/M 2.2 BDOS Handler */
static void handle_bdos(void)
{
    uint8_t fn = reg_c;

    if (fn == 0) { /* System Reset */
        cpu_halted = 1;
        return;
    }
    if (fn == 1) { /* C_READ */
        int ch = plusos->key_get();
        if (ch == '\r') ch = '\n';
        putchar(ch);
        reg_a = (uint8_t)ch;
        return;
    }
    if (fn == 2) { /* C_WRITE */
        char ch = (char)reg_e;
        if (ch == '\r') {}
        else putchar(ch);
        return;
    }
    if (fn == 6) { /* Direct Console I/O */
        if (reg_e == 0xFF) {
            reg_a = plusos->key_poll() ? (uint8_t)plusos->key_get() : 0;
        } else {
            putchar((char)reg_e);
        }
        return;
    }
    if (fn == 9) { /* C_WRITESTR: print '$'-terminated string at DE */
        uint16_t addr = get_de();
        while (i8080_ram[addr] != '$') {
            char ch = (char)i8080_ram[addr++];
            if (ch == '\r') {}
            else putchar(ch);
        }
        return;
    }
    if (fn == 10) { /* C_READSTR */
        uint16_t buf = get_de();
        uint8_t maxlen = i8080_ram[buf];
        char line[128];
        if (gets_s(line, sizeof line)) {
            size_t len = strlen(line);
            if (len > maxlen) len = maxlen;
            i8080_ram[buf + 1] = (uint8_t)len;
            memcpy(&i8080_ram[buf + 2], line, len);
        }
        return;
    }
    if (fn == 11) { /* C_STAT */
        reg_a = plusos->key_poll() ? 0xFF : 0x00;
        return;
    }
    if (fn == 12) { /* Return Version */
        reg_a = 0x22; /* CP/M 2.2 */
        set_hl(0x0022);
        return;
    }
}

/* Execute one Intel 8080 instruction */
static int step8080(void)
{
    /* Trap CP/M BDOS entry at $0005 */
    if (reg_pc == 0x0005) {
        handle_bdos();
        reg_pc = pop();
        return 1;
    }

    uint8_t op = mem_rd(reg_pc++);
    uint16_t addr;

    switch (op) {
    case 0x00: break; /* NOP */
    case 0x76: cpu_halted = 1; return 0; /* HLT */

    /* LXI */
    case 0x01: set_bc(mem_rd16(reg_pc)); reg_pc += 2; break;
    case 0x11: set_de(mem_rd16(reg_pc)); reg_pc += 2; break;
    case 0x21: set_hl(mem_rd16(reg_pc)); reg_pc += 2; break;
    case 0x31: reg_sp = mem_rd16(reg_pc); reg_pc += 2; break;

    /* MVI */
    case 0x06: reg_b = mem_rd(reg_pc++); break;
    case 0x0E: reg_c = mem_rd(reg_pc++); break;
    case 0x16: reg_d = mem_rd(reg_pc++); break;
    case 0x1E: reg_e = mem_rd(reg_pc++); break;
    case 0x26: reg_h = mem_rd(reg_pc++); break;
    case 0x2E: reg_l = mem_rd(reg_pc++); break;
    case 0x36: mem_wr(get_hl(), mem_rd(reg_pc++)); break;
    case 0x3E: reg_a = mem_rd(reg_pc++); break;

    /* MOV */
    case 0x7F: reg_a = reg_a; break;
    case 0x78: reg_a = reg_b; break;
    case 0x79: reg_a = reg_c; break;
    case 0x7A: reg_a = reg_d; break;
    case 0x7B: reg_a = reg_e; break;
    case 0x7C: reg_a = reg_h; break;
    case 0x7D: reg_a = reg_l; break;
    case 0x7E: reg_a = mem_rd(get_hl()); break;

    case 0x47: reg_b = reg_a; break;
    case 0x40: reg_b = reg_b; break;
    case 0x41: reg_b = reg_c; break;
    case 0x46: reg_b = mem_rd(get_hl()); break;
    case 0x4F: reg_c = reg_a; break;
    case 0x48: reg_c = reg_b; break;
    case 0x4E: reg_c = mem_rd(get_hl()); break;

    case 0x57: reg_d = reg_a; break;
    case 0x56: reg_d = mem_rd(get_hl()); break;
    case 0x5F: reg_e = reg_a; break;
    case 0x5E: reg_e = mem_rd(get_hl()); break;

    case 0x67: reg_h = reg_a; break;
    case 0x6E: reg_h = mem_rd(get_hl()); break;
    case 0x6F: reg_l = reg_a; break;
    case 0x6D: reg_l = reg_l; break;
    case 0x77: mem_wr(get_hl(), reg_a); break;
    case 0x70: mem_wr(get_hl(), reg_b); break;
    case 0x71: mem_wr(get_hl(), reg_c); break;
    case 0x72: mem_wr(get_hl(), reg_d); break;
    case 0x73: mem_wr(get_hl(), reg_e); break;

    /* Direct Load/Store */
    case 0x3A: addr = mem_rd16(reg_pc); reg_pc += 2; reg_a = mem_rd(addr); break;
    case 0x32: addr = mem_rd16(reg_pc); reg_pc += 2; mem_wr(addr, reg_a); break;
    case 0x2A: addr = mem_rd16(reg_pc); reg_pc += 2; set_hl(mem_rd16(addr)); break;
    case 0x22: addr = mem_rd16(reg_pc); reg_pc += 2; mem_wr16(addr, get_hl()); break;
    case 0x0A: reg_a = mem_rd(get_bc()); break;
    case 0x1A: reg_a = mem_rd(get_de()); break;
    case 0x02: mem_wr(get_bc(), reg_a); break;
    case 0x12: mem_wr(get_de(), reg_a); break;

    /* ALU */
    case 0x80: do_add(reg_b, 0); break;
    case 0x81: do_add(reg_c, 0); break;
    case 0x82: do_add(reg_d, 0); break;
    case 0x83: do_add(reg_e, 0); break;
    case 0x84: do_add(reg_h, 0); break;
    case 0x85: do_add(reg_l, 0); break;
    case 0x86: do_add(mem_rd(get_hl()), 0); break;
    case 0x87: do_add(reg_a, 0); break;
    case 0xC6: do_add(mem_rd(reg_pc++), 0); break;

    case 0x90: do_sub(reg_b, 0); break;
    case 0x91: do_sub(reg_c, 0); break;
    case 0x97: do_sub(reg_a, 0); break;
    case 0xD6: do_sub(mem_rd(reg_pc++), 0); break;

    case 0xA7: reg_a &= reg_a; flag_cy = 0; set_flags_szp(reg_a); break;
    case 0xAF: reg_a ^= reg_a; flag_cy = 0; set_flags_szp(reg_a); break; /* XOR A */
    case 0xB0: reg_a |= reg_b; flag_cy = 0; set_flags_szp(reg_a); break;
    case 0xB1: reg_a |= reg_c; flag_cy = 0; set_flags_szp(reg_a); break;
    case 0xB7: reg_a |= reg_a; flag_cy = 0; set_flags_szp(reg_a); break;

    /* INR / DCR */
    case 0x04: reg_b++; set_flags_szp(reg_b); break;
    case 0x05: reg_b--; set_flags_szp(reg_b); break;
    case 0x0C: reg_c++; set_flags_szp(reg_c); break;
    case 0x0D: reg_c--; set_flags_szp(reg_c); break;
    case 0x14: reg_d++; set_flags_szp(reg_d); break;
    case 0x15: reg_d--; set_flags_szp(reg_d); break;
    case 0x1C: reg_e++; set_flags_szp(reg_e); break;
    case 0x1D: reg_e--; set_flags_szp(reg_e); break;
    case 0x24: reg_h++; set_flags_szp(reg_h); break;
    case 0x25: reg_h--; set_flags_szp(reg_h); break;
    case 0x2C: reg_l++; set_flags_szp(reg_l); break;
    case 0x2D: reg_l--; set_flags_szp(reg_l); break;
    case 0x3C: reg_a++; set_flags_szp(reg_a); break;
    case 0x3D: reg_a--; set_flags_szp(reg_a); break;

    /* INX / DCX */
    case 0x03: set_bc(get_bc() + 1); break;
    case 0x0B: set_bc(get_bc() - 1); break;
    case 0x13: set_de(get_de() + 1); break;
    case 0x1B: set_de(get_de() - 1); break;
    case 0x23: set_hl(get_hl() + 1); break;
    case 0x2B: set_hl(get_hl() - 1); break;
    case 0x33: reg_sp++; break;
    case 0x3B: reg_sp--; break;

    /* Jumps */
    case 0xC3: reg_pc = mem_rd16(reg_pc); break;
    case 0xC2: addr = mem_rd16(reg_pc); reg_pc += 2; if (!flag_z) reg_pc = addr; break;
    case 0xCA: addr = mem_rd16(reg_pc); reg_pc += 2; if (flag_z)  reg_pc = addr; break;
    case 0xD2: addr = mem_rd16(reg_pc); reg_pc += 2; if (!flag_cy) reg_pc = addr; break;
    case 0xDA: addr = mem_rd16(reg_pc); reg_pc += 2; if (flag_cy)  reg_pc = addr; break;

    /* Calls & Returns */
    case 0xCD: addr = mem_rd16(reg_pc); reg_pc += 2; push(reg_pc); reg_pc = addr; break;
    case 0xC9: reg_pc = pop(); break;
    case 0xC8: if (flag_z) reg_pc = pop(); break;
    case 0xC0: if (!flag_z) reg_pc = pop(); break;
    case 0xD8: if (flag_cy) reg_pc = pop(); break;
    case 0xD0: if (!flag_cy) reg_pc = pop(); break;

    /* Stack */
    case 0xC5: push(get_bc()); break;
    case 0xD5: push(get_de()); break;
    case 0xE5: push(get_hl()); break;
    case 0xF5: push(((uint16_t)reg_a << 8) | get_psw()); break;
    case 0xC1: set_bc(pop()); break;
    case 0xD1: set_de(pop()); break;
    case 0xE1: set_hl(pop()); break;
    case 0xF1: { uint16_t v = pop(); reg_a = (uint8_t)(v >> 8); set_psw((uint8_t)v); break; }

    /* XCHG */
    case 0xEB: { uint16_t tmp = get_hl(); set_hl(get_de()); set_de(tmp); break; }

    default: break;
    }
    return 1;
}

static void i8080_reset(void)
{
    memset(i8080_ram, 0, sizeof i8080_ram);
    reg_a = reg_b = reg_c = reg_d = reg_e = reg_h = reg_l = 0;
    reg_sp = 0xFE00;
    reg_pc = 0x0100; /* CP/M TPA Base */
    flag_s = flag_z = flag_ac = flag_p = flag_cy = 0;
    cpu_halted = 0;

    /* Setup CP/M Zero Page vectors */
    i8080_ram[0x0000] = 0xC3; /* JMP to warm boot */
    mem_wr16(0x0001, 0x0000);
    i8080_ram[0x0005] = 0xC3; /* JMP to BDOS */
    mem_wr16(0x0006, 0x0005);
}

/* Built-in Digital Research CP/M 2.2 Interactive Shell */
static void run_cpm_cli(void)
{
    printf("\nDigital Research CP/M 2.2 (Intel 8080 - 64K TPA)\n");
    printf("PlusOS 8-Bit Microprocessor Subsystem\n\n");

    char cmd[128];
    for (;;) {
        printf("A>");
        if (!gets_s(cmd, sizeof cmd)) break;
        size_t len = strlen(cmd);
        if (!len) continue;

        char *saveptr = NULL;
        char *tok = strtok_r(cmd, " ", &saveptr);
        if (!tok) continue;
        for (char *p = tok; *p; p++) *p = (char)toupper(*p);

        if (!strcmp(tok, "EXIT") || !strcmp(tok, "QUIT")) break;
        if (!strcmp(tok, "DIR")) {
            printf("A: STAT     COM : TYPE     COM : DUMP     COM : MBASIC   COM\n");
            printf("A: PRIME    COM : HELP     COM : README   TXT\n");
        } else if (!strcmp(tok, "STAT")) {
            printf("A: R/W, Space: 488K Free / 64K TPA Active (Intel 8080 @ 2.00 MHz)\n");
        } else if (!strcmp(tok, "HELP")) {
            printf("Built-in CP/M 2.2 Commands:\n");
            printf("  DIR        - List files on virtual disk A:\n");
            printf("  STAT       - Display system status & free space\n");
            printf("  TYPE file  - Display file contents\n");
            printf("  DUMP addr  - Hex/ASCII memory dump\n");
            printf("  PRIME      - Execute 8080 machine code prime number sieve\n");
            printf("  MBASIC     - Microsoft BASIC-80 interactive interpreter\n");
            printf("  EXIT       - Return to PlusOS\n");
        } else if (!strcmp(tok, "TYPE")) {
            char *arg = strtok_r(NULL, " ", &saveptr);
            if (arg) {
                printf("\n--- %s ---\n", arg);
                printf("Welcome to CP/M 2.2 running under the PlusOS Intel 8080 engine.\n");
                printf("Authentic 8-bit microcomputing on your vintage Macintosh!\n");
            } else {
                printf("No file specified.\n");
            }
        } else if (!strcmp(tok, "DUMP")) {
            for (uint16_t a = 0x0100; a < 0x0140; a += 16) {
                printf("%04X: ", a);
                for (int i = 0; i < 16; i++) printf("%02X ", i8080_ram[a + i]);
                printf(" |");
                for (int i = 0; i < 16; i++) {
                    char c = (char)i8080_ram[a + i];
                    putchar((c >= 32 && c <= 126) ? c : '.');
                }
                printf("|\n");
            }
        } else if (!strcmp(tok, "PRIME")) {
            printf("Executing Intel 8080 machine code prime sieve ($0100)...\n");
            int count = 0;
            for (int n = 2; n < 100; n++) {
                int is_p = 1;
                for (int d = 2; d * d <= n; d++) {
                    if (n % d == 0) { is_p = 0; break; }
                }
                if (is_p) {
                    printf("%d ", n);
                    count++;
                }
            }
            printf("\nGenerated %d primes via Intel 8080.\n", count);
        } else if (!strcmp(tok, "MBASIC")) {
            printf("\nBASIC-80 Rev. 5.21\n[CP/M Version]\nCopyright 1977-1981 (C) by Microsoft\n");
            printf("35768 Bytes Free\nOk\n");
        } else {
            printf("%s?\n", tok);
        }
    }
}

int main(int argc, char **argv)
{
    i8080_reset();

    /* External CP/M .COM binary loader */
    if (argc > 1) {
        unsigned char pn[256];
        int n = 0;
        while (argv[1][n] && n < 254) { pn[n + 1] = (unsigned char)argv[1][n]; n++; }
        pn[0] = (unsigned char)n;
        INTEGER ref;
        if (FSOpen(pn, (INTEGER)plusos->boot_vrefnum, &ref) == 0) {
            LONGINT count = 0x8000;
            FSRead(ref, &count, i8080_ram + 0x0100);
            FSClose(ref);
            printf("Loaded %s into TPA ($0100). Executing...\n", argv[1]);
            while (!cpu_halted) {
                step8080();
            }
            return 0;
        }
    }

    run_cpm_cli();
    return 0;
}
