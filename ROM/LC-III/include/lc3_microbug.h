/* lc3_microbug.h - Macintosh LC III Built-in ROM Debugger (MicroBug)
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x002AB6 - 0x002DCE
 */
#ifndef LC3_MICROBUG_H
#define LC3_MICROBUG_H

#include <stdint.h>
#include <stdbool.h>

/* CPU Register Frame captured on debugger entry */
typedef struct {
    uint32_t d[8];      /* D0 - D7 */
    uint32_t a[8];      /* A0 - A7 (SP) */
    uint16_t sr;        /* Status Register */
    uint32_t pc;        /* Program Counter */
    uint32_t usp;       /* User Stack Pointer */
    uint32_t ssp;       /* Supervisor Stack Pointer */
    uint32_t vbr;       /* Vector Base Register */
    uint32_t cacr;      /* Cache Control Register */
} LC3DbgRegisters;

/* Debugger Command Table Entry */
typedef struct {
    char     cmd[4];
    void   (*handler)(const char *args);
    const char *help;
} LC3DbgCmd;

/* Public MicroBug Interface */
void     lc3_microbug_enter(LC3DbgRegisters *regs, const char *reason);
void     lc3_microbug_prompt(void);
void     lc3_microbug_print(const char *str);
void     lc3_microbug_print_hex_nibble(uint8_t n);
void     lc3_microbug_print_hex_byte(uint8_t b);
void     lc3_microbug_print_hex_word(uint16_t w);
void     lc3_microbug_print_hex_long(uint32_t l);
uint8_t  lc3_microbug_ascii_to_nibble(char c);
uint32_t lc3_microbug_eval_expr(const char **exprStr);

/* MicroBug Built-in Commands */
void     lc3_microbug_cmd_dm(const char *args);   /* Dump Memory */
void     lc3_microbug_cmd_sm(const char *args);   /* Set Memory */
void     lc3_microbug_cmd_dr(const char *args);   /* Display Registers */
void     lc3_microbug_cmd_sr(const char *args);   /* Set Register */
void     lc3_microbug_cmd_g(const char *args);    /* Go / Resume execution */

#endif /* LC3_MICROBUG_H */
