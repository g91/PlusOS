/* lc3_vbl_interrupts.h - Macintosh LC III VBL & Interrupt Demultiplexer Engine
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x009D30 - 0x00A2BA
 */
#ifndef LC3_VBL_INTERRUPTS_H
#define LC3_VBL_INTERRUPTS_H

#include <stdint.h>
#include <stdbool.h>

/* VBL Task Queue Element */
typedef struct LC3VBLTask {
    struct LC3VBLTask *qLink;
    uint16_t           qType;
    void             (*vblAddr)(void);
    int16_t            vblCount;
    int16_t            vblPhase;
} LC3VBLTask;

/* VBL Queue Header */
typedef struct {
    uint16_t    qFlags;
    LC3VBLTask *qHead;
    LC3VBLTask *qTail;
} LC3VBLQueueHeader;

/* Low-Memory Globals */
#define LM_VBLQUEUE          (*(volatile LC3VBLQueueHeader *)0x0160)

/* VBL & Interrupt API */
void    lc3_vbl_init(void);
int16_t lc3_vbl_install(LC3VBLTask *vblTask);
int16_t lc3_vbl_remove(LC3VBLTask *vblTask);
void    lc3_vbl_interrupt_handler(void);
void    lc3_scc_exta_interrupt(void);
void    lc3_scc_extb_interrupt(void);
void    lc3_ext_status_dispatch(uint8_t channel);

#endif /* LC3_VBL_INTERRUPTS_H */
