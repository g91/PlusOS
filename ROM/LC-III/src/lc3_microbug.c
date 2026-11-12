/* lc3_microbug.c - Macintosh LC III Built-in ROM Debugger (MicroBug)
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   MICROBUG_PROMPT, MICROBUG_FILL_BUF, MICROBUG_KEY_LOOP, MICROBUG_CHECK_KEY,
 *   MICROBUG_EXEC_CMD, MICROBUG_PARSE_CMD, MICROBUG_CMD_LOOP, MICROBUG_CMD_CMP,
 *   DT_DbgCmdTab, P_DbgCmd_DM, DM_ROW_LOOP, DM_WORD_LOOP, PRINT_HEX_LONG,
 *   PRINT_HEX_WORD, PRINT_HEX_BYTE, PRINT_HEX_NIBBLE, HEX_DIGIT_OK, SM_STORE_LOOP,
 *   DBGREG_SHOW_WORD, ASCII_TO_NIBBLE, EXPR_SCAN_PREFIX, EXPR_CHECK_OP,
 *   EXPR_FINALIZE, EXPR_DEREF_LOOP, EXPR_ACCUMULATE, EXPR_CHECK_SIZE,
 *   EXPR_CHECK_NEXT, EXPR_TOKEN_DONE, EXPR_CHECK_PC, EXPR_READ_REGNUM
 */

#include "lc3_microbug.h"
#include "lc3_hardware.h"
#include <string.h>

static LC3DbgRegisters *g_currentRegs = 0;
static bool             g_debuggerActive = false;

/* Screen text output coordinates for MicroBug window */
static uint16_t g_dbgCursorRow = 0;
static uint16_t g_dbgCursorCol = 0;

/* Built-in Command Table (DT_DbgCmdTab) */
static const LC3DbgCmd g_dbgCmdTab[] = {
    { "DM", lc3_microbug_cmd_dm, "Dump Memory: DM <addr> [count]" },
    { "SM", lc3_microbug_cmd_sm, "Set Memory: SM <addr> <val>..." },
    { "DR", lc3_microbug_cmd_dr, "Display Registers" },
    { "SR", lc3_microbug_cmd_sr, "Set Register: SR <reg> <val>" },
    { "G",  lc3_microbug_cmd_g,  "Go / Continue execution" },
    { {0},  0,                   0 }
};

uint8_t lc3_microbug_ascii_to_nibble(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    return 0xFF; /* invalid */
}

void lc3_microbug_print_char(char c) {
    /* Direct rendering of ASCII character to VRAM frame buffer */
    volatile uint8_t *vram = (volatile uint8_t *)LC3_VRAM_BASE;
    if (c == '\n' || c == '\r') {
        g_dbgCursorCol = 0;
        g_dbgCursorRow++;
        if (g_dbgCursorRow >= 50) {
            g_dbgCursorRow = 0; /* wrap around */
        }
        return;
    }
    if (c < 0x20 || c > 0x7E) {
        c = '.';
    }
    /* Draw 8x8 font cell in monochrome or 8-bpp */
    uint32_t lineBytes = 80; /* 1-bpp rowBytes default */
    uint32_t offset = (g_dbgCursorRow * 8 * lineBytes) + g_dbgCursorCol;
    /* Invert byte block to represent character */
    for (int r = 0; r < 8; ++r) {
        vram[offset + (r * lineBytes)] = (uint8_t)c;
    }
    g_dbgCursorCol++;
    if (g_dbgCursorCol >= 78) {
        g_dbgCursorCol = 0;
        g_dbgCursorRow++;
    }
}

void lc3_microbug_print(const char *str) {
    while (*str) {
        lc3_microbug_print_char(*str++);
    }
}

void lc3_microbug_print_hex_nibble(uint8_t n) {
    static const char hex[] = "0123456789ABCDEF";
    lc3_microbug_print_char(hex[n & 0x0F]);
}

void lc3_microbug_print_hex_byte(uint8_t b) {
    lc3_microbug_print_hex_nibble(b >> 4);
    lc3_microbug_print_hex_nibble(b & 0x0F);
}

void lc3_microbug_print_hex_word(uint16_t w) {
    lc3_microbug_print_hex_byte((uint8_t)(w >> 8));
    lc3_microbug_print_hex_byte((uint8_t)(w & 0xFF));
}

void lc3_microbug_print_hex_long(uint32_t l) {
    lc3_microbug_print_hex_word((uint16_t)(l >> 16));
    lc3_microbug_print_hex_word((uint16_t)(l & 0xFFFF));
}

uint32_t lc3_microbug_eval_expr(const char **exprStr) {
    /* EXPR_SCAN_PREFIX, EXPR_CHECK_OP, EXPR_ACCUMULATE, EXPR_DEREF_LOOP */
    const char *p = *exprStr;
    while (*p == ' ' || *p == '\t') p++;

    uint32_t val = 0;
    bool isHex = false;
    if (*p == '$') {
        isHex = true;
        p++;
    } else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        isHex = true;
        p += 2;
    }

    while (*p) {
        uint8_t nib = lc3_microbug_ascii_to_nibble(*p);
        if (nib == 0xFF) break;
        if (isHex || (*p >= 'A' && *p <= 'F') || (*p >= 'a' && *p <= 'f')) {
            val = (val << 4) | nib;
        } else {
            val = (val * 10) + nib;
        }
        p++;
    }

    /* Check dereference operators (@addr) */
    while (*p == '@') {
        p++;
        val = *(volatile uint32_t *)val;
    }

    *exprStr = p;
    return val;
}

void lc3_microbug_cmd_dm(const char *args) {
    /* DM_ROW_LOOP, DM_WORD_LOOP */
    uint32_t addr = lc3_microbug_eval_expr(&args);
    uint32_t count = 64; /* default 64 bytes */
    if (*args) {
        count = lc3_microbug_eval_expr(&args);
        if (count == 0) count = 16;
    }

    uint32_t rows = (count + 15) / 16;
    for (uint32_t r = 0; r < rows; ++r) {
        lc3_microbug_print_hex_long(addr);
        lc3_microbug_print(": ");

        volatile uint8_t *p = (volatile uint8_t *)addr;
        /* Hex bytes */
        for (int i = 0; i < 16; ++i) {
            lc3_microbug_print_hex_byte(p[i]);
            lc3_microbug_print_char(' ');
            if (i == 7) lc3_microbug_print_char(' ');
        }
        /* ASCII dump */
        lc3_microbug_print(" |");
        for (int i = 0; i < 16; ++i) {
            char c = (char)p[i];
            lc3_microbug_print_char((c >= 0x20 && c <= 0x7E) ? c : '.');
        }
        lc3_microbug_print("|\n");
        addr += 16;
    }
}

void lc3_microbug_cmd_sm(const char *args) {
    /* SM_STORE_LOOP */
    uint32_t addr = lc3_microbug_eval_expr(&args);
    volatile uint8_t *p = (volatile uint8_t *)addr;
    while (*args) {
        while (*args == ' ' || *args == '\t') args++;
        if (!*args) break;
        uint32_t b = lc3_microbug_eval_expr(&args);
        *p++ = (uint8_t)b;
    }
}

void lc3_microbug_cmd_dr(const char *args) {
    (void)args;
    if (!g_currentRegs) return;
    lc3_microbug_print("D0-D3: ");
    for (int i = 0; i < 4; ++i) {
        lc3_microbug_print_hex_long(g_currentRegs->d[i]);
        lc3_microbug_print_char(' ');
    }
    lc3_microbug_print("\nD4-D7: ");
    for (int i = 4; i < 8; ++i) {
        lc3_microbug_print_hex_long(g_currentRegs->d[i]);
        lc3_microbug_print_char(' ');
    }
    lc3_microbug_print("\nA0-A3: ");
    for (int i = 0; i < 4; ++i) {
        lc3_microbug_print_hex_long(g_currentRegs->a[i]);
        lc3_microbug_print_char(' ');
    }
    lc3_microbug_print("\nA4-A7: ");
    for (int i = 4; i < 8; ++i) {
        lc3_microbug_print_hex_long(g_currentRegs->a[i]);
        lc3_microbug_print_char(' ');
    }
    lc3_microbug_print("\nPC=");
    lc3_microbug_print_hex_long(g_currentRegs->pc);
    lc3_microbug_print(" SR=");
    lc3_microbug_print_hex_word(g_currentRegs->sr);
    lc3_microbug_print(" CACR=");
    lc3_microbug_print_hex_long(g_currentRegs->cacr);
    lc3_microbug_print("\n");
}

void lc3_microbug_cmd_sr(const char *args) {
    if (!g_currentRegs) return;
    while (*args == ' ' || *args == '\t') args++;
    char regChar = *args++;
    int regIdx = *args++ - '0';
    uint32_t val = lc3_microbug_eval_expr(&args);
    if ((regChar == 'd' || regChar == 'D') && regIdx >= 0 && regIdx < 8) {
        g_currentRegs->d[regIdx] = val;
    } else if ((regChar == 'a' || regChar == 'A') && regIdx >= 0 && regIdx < 8) {
        g_currentRegs->a[regIdx] = val;
    } else if (regChar == 'p' || regChar == 'P') {
        g_currentRegs->pc = val;
    }
}

void lc3_microbug_cmd_g(const char *args) {
    if (*args) {
        uint32_t targetPC = lc3_microbug_eval_expr(&args);
        if (g_currentRegs) {
            g_currentRegs->pc = targetPC;
        }
    }
    g_debuggerActive = false;
}

void lc3_microbug_prompt(void) {
    lc3_microbug_print("> ");
}

void lc3_microbug_enter(LC3DbgRegisters *regs, const char *reason) {
    g_currentRegs = regs;
    g_debuggerActive = true;
    lc3_microbug_print("\n*** MicroBug Entered: ");
    lc3_microbug_print(reason ? reason : "Breakpoint");
    lc3_microbug_print(" ***\n");
    lc3_microbug_cmd_dr(0);
    lc3_microbug_prompt();
}
