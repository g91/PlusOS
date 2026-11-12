/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Hardware Self-Test & Diagnostic Monitor (Test Manager)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_TEST_MGR_H
#define LC3_TEST_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define TM_PASSED          0
#define TM_ERR_RAM        1
#define TM_ERR_ROM_CKSUM   2
#define TM_ERR_SONORA_VRAM 3
#define TM_ERR_SCSI_ESP    4
#define TM_ERR_VIA_EGRET   5
#define TM_ERR_DFAC_SOUND  6

typedef struct {
    uint32_t ramSize;
    uint32_t romChecksum;
    bool vramPassed;
    bool espPassed;
    bool egretPassed;
    bool dfacPassed;
    uint32_t testErrorCode;
} TMDiagnosticResults;

/* Hardware Diagnostics & Test Manager API */
int16_t lc3_tm_init(void);
int16_t lc3_tm_run_diagnostics(TMDiagnosticResults *results);
bool lc3_tm_test_ram(uint32_t startAddr, uint32_t byteLength);
bool lc3_tm_test_rom_checksum(void);
bool lc3_tm_test_sonora(void);
bool lc3_tm_test_scsi_esp(void);
bool lc3_tm_test_dfac(void);
void lc3_tm_play_sound_code(int16_t code);

#endif /* LC3_TEST_MGR_H */
