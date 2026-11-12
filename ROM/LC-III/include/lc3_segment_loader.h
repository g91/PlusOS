/* lc3_segment_loader.h - Macintosh LC III Segment Loader & Application Launch Engine
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x00BCE0 - 0x00BFC0
 */
#ifndef LC3_SEGMENT_LOADER_H
#define LC3_SEGMENT_LOADER_H

#include <stdint.h>
#include <stdbool.h>

/* Jump Table Entry in Application A5 World */
typedef struct {
    uint16_t jmpInst;           /* 0x4EF9 (JMP long) */
    uint32_t jmpTarget;         /* Target address in loaded segment */
    uint16_t segNumber;         /* 'CODE' resource ID */
    uint16_t loadSegTrap;       /* 0xA9F0 (_LoadSeg) */
} LC3JumpTableEntry;

/* App Parameters Record */
typedef struct {
    char     apName[32];        /* Pascal string of app name */
    int16_t  apRefNum;          /* Application resource fork refNum */
    uint32_t apParam;           /* Finder launch parameters */
} LC3AppParams;

/* Segment Loader Low-Memory Globals */
#define LM_CURAPNAME         ((char *)0x0910)
#define LM_CURAPREFNUM       (*(volatile int16_t  *)0x0900)
#define LM_CURSTACKBASE      (*(volatile uint32_t *)0x0908)
#define LM_APPLSCRATCH       ((uint8_t *)0x0A78)
#define LM_CURRENT_A5        (*(volatile uint32_t *)0x0904)

/* Segment Loader API */
void     lc3_segloader_init(void);
void     lc3_load_seg(int16_t segNumber);
void     lc3_unload_seg(void *segAddr);
void     lc3_chain(const char *appName);
void     lc3_exit_to_shell(void);
void     lc3_get_app_params(LC3AppParams *params);
void     lc3_launch(const char *appName, uint16_t launchFlags);

#endif /* LC3_SEGMENT_LOADER_H */
