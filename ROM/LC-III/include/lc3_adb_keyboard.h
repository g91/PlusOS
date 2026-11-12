/* lc3_adb_keyboard.h - Macintosh LC III ADB & Keyboard Transceiver Engine
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x00A360 - 0x00AE7E
 */
#ifndef LC3_ADB_KEYBOARD_H
#define LC3_ADB_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

#define ADB_MAX_DEVICES 16

/* ADB Device Information Record */
typedef struct {
    uint8_t  devType;           /* Original ADB device address */
    uint8_t  origAddr;          /* Default device address */
    void   (*dbServiceRtPtr)(void *data, uint8_t byteCount);
    void    *dbDataAreaAddr;    /* Optional data buffer */
} LC3ADBDevInfo;

/* ADB Device Table Entry in Low Memory */
typedef struct {
    LC3ADBDevInfo devInfo;
    uint8_t       currentAddr;
    bool          inUse;
} LC3ADBTableEntry;

/* ADB Low-Memory Globals */
#define LM_KEYMAP            ((uint32_t *)0x0174) /* 16-byte key state bitmap */
#define LM_KEYLAST           (*(volatile uint16_t *)0x0184)
#define LM_ADBDELAY          (*(volatile uint16_t *)0x0CF8)

/* ADB API */
void     lc3_adb_init(void);
int16_t  lc3_adb_reinit(void);
int16_t  lc3_adb_count(void);
int16_t  lc3_adb_get_ind_info(LC3ADBDevInfo *info, int16_t devIndex);
int16_t  lc3_adb_get_info(uint8_t adbAddr, LC3ADBDevInfo *info);
int16_t  lc3_adb_set_info(uint8_t adbAddr, const LC3ADBDevInfo *info);
int16_t  lc3_adb_op(void *buffer, void (*compRtn)(void), void *data, int16_t commandNum);

/* Keyboard Engine & KeyTrans */
void     lc3_kbd_update_keymap(uint8_t rawKeyCode, bool isKeyDown);
void     lc3_kbd_set_led(uint8_t ledMask);
uint32_t lc3_kbd_keytrans(const void *transData, uint16_t keyCode, uint32_t *state);

#endif /* LC3_ADB_KEYBOARD_H */
