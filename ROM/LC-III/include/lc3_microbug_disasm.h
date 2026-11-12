/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: MicroBug Interactive Disassembler & 68030 Decoder
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_MICROBUG_DISASM_H
#define LC3_MICROBUG_DISASM_H

#include <stdint.h>
#include <stdbool.h>

#define DISASM_NO_ERR         0
#define DISASM_PARAM_ERR    -50

typedef struct DisasmInstruction {
    uint32_t address;
    uint16_t opcode;
    uint8_t byteLength;
    char mnemonic[16];
    char operands[48];
} DisasmInstruction;

/* MicroBug Disassembler API */
int16_t lc3_disasm_68030(const uint8_t *code, uint32_t addr, DisasmInstruction *instr);
int16_t lc3_disasm_decode_ea(uint8_t mode, uint8_t reg, uint8_t size, const uint8_t *stream, char *outStr, uint8_t *extraBytes);
int16_t lc3_disasm_format_movem(uint16_t regMask, bool isPredecrement, char *outStr);
int16_t lc3_disasm_decode_branch(uint32_t currentAddr, int16_t disp, uint32_t *targetAddr);
int16_t lc3_disasm_decode_bitfield(uint16_t extWord, char *outStr);
int16_t lc3_disasm_print_line(const DisasmInstruction *instr, char *lineBuffer, uint16_t maxLen);

#endif /* LC3_MICROBUG_DISASM_H */
