/* lc3_syserr_alert.h - Macintosh LC III System Error & Alert Manager
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x002792 - 0x002A38
 */
#ifndef LC3_SYSERR_ALERT_H
#define LC3_SYSERR_ALERT_H

#include <stdint.h>
#include <stdbool.h>

#define DS_BOMB_ALERT        1
#define DS_OUT_OF_MEM        2
#define DS_UNIMPLEMENTED     12
#define DS_BAD_TRAP          13
#define DS_BUS_ERROR         1
#define DS_ADDR_ERROR        2
#define DS_ILLEGAL_INST      3
#define DS_ZERO_DIVIDE       4

/* Alert Button Record */
typedef struct {
    int16_t  top;
    int16_t  left;
    int16_t  bottom;
    int16_t  right;
    char     title[16];
    bool     isDefault;
} LC3AlertButton;

/* System Error Handler State */
typedef struct {
    int16_t        dsAlertID;
    const char    *errorMsg;
    uint32_t       faultPC;
    uint32_t       faultAddr;
    LC3AlertButton buttons[3];
    uint8_t        buttonCount;
} LC3SysErrState;

/* System Error Low-Memory Globals */
#define LM_DSERRCODE         (*(volatile int16_t *)0x02BA)
#define LM_DSALERTTAB        (*(volatile uint32_t *)0x02BC)

/* System Error Interface */
void    lc3_syserr_init(void);
void    lc3_syserr_dispatch(int16_t errorCode, uint32_t faultPC, uint32_t faultAddr);
bool    lc3_syserr_try_debugger(int16_t errorCode);
void    lc3_syserr_draw_alert_frame(int16_t top, int16_t left, int16_t bottom, int16_t right);
void    lc3_syserr_draw_buttons(const LC3AlertButton *btns, uint8_t count);
void    lc3_syserr_btn_invert(const LC3AlertButton *btn);
int16_t lc3_syserr_wait_mouse_down(const LC3AlertButton *btns, uint8_t count);

#endif /* LC3_SYSERR_ALERT_H */
