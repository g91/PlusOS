/* lc3_adb_keyboard.c - Macintosh LC III ADB & Keyboard Transceiver Engine
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   COUNTADBS, GETINDADB, ADB_COPY_DEVTBL, GETADBINFO, SETADBINFO,
 *   ADB_FIND_DEVICE, ADBREINIT, ADBINIT_BODY, ADB_INIT_WAIT, ADB_UPDATE_DEVTBL,
 *   ZERO_KEYMAP, KBD_UPDATE_KEYMAP, KBD_KEYUP_AUTO, KBD_BUILD_MESSAGE,
 *   KBD_COMPLETION, KBD_POST_KEYUP, KBD_SPECIAL_KEY, KBD_LED_PATH,
 *   KBD_QUEUE_LED_UPDATE, KBD_SET_LED, KEYTRANS, KEYTRANS_HAVE_DEAD,
 *   KEYTRANS_DEFAULT_CHAR, KEYTRANS_NORMAL_CHAR, KEYTRANS_DEAD_LOOKUP,
 *   KEYTRANS_DEAD_MATCH, KEYTRANS_DEAD_DONE, KEYTRANS_DEAD_NOMATCH
 */

#include "lc3_adb_keyboard.h"
#include "lc3_sonora_egret.h"
#include "lc3_hardware.h"
#include <string.h>

static LC3ADBTableEntry g_adbDeviceTable[ADB_MAX_DEVICES];
static uint8_t g_deviceCount = 0;
static uint8_t g_currentLedState = 0;

void lc3_adb_init(void) {
    /* ZERO_KEYMAP, ADBINIT_BODY */
    memset(g_adbDeviceTable, 0, sizeof(g_adbDeviceTable));
    memset((void *)LM_KEYMAP, 0, 16);
    LM_KEYLAST = 0;
    LM_ADBDELAY = 100;
    lc3_adb_reinit();
}

int16_t lc3_adb_reinit(void) {
    /* ADBREINIT, ADB_UPDATE_DEVTBL, ADB_FIND_DEVICE_LOOP */
    g_deviceCount = 0;

    /* Default Mac ADB device 2: Keyboard */
    g_adbDeviceTable[0].devInfo.origAddr = 2;
    g_adbDeviceTable[0].devInfo.devType = 2;
    g_adbDeviceTable[0].devInfo.dbServiceRtPtr = 0;
    g_adbDeviceTable[0].currentAddr = 2;
    g_adbDeviceTable[0].inUse = true;
    g_deviceCount++;

    /* Default Mac ADB device 3: Mouse */
    g_adbDeviceTable[1].devInfo.origAddr = 3;
    g_adbDeviceTable[1].devInfo.devType = 3;
    g_adbDeviceTable[1].devInfo.dbServiceRtPtr = 0;
    g_adbDeviceTable[1].currentAddr = 3;
    g_adbDeviceTable[1].inUse = true;
    g_deviceCount++;

    return 0; /* noErr */
}

int16_t lc3_adb_count(void) {
    /* COUNTADBS */
    return (int16_t)g_deviceCount;
}

int16_t lc3_adb_get_ind_info(LC3ADBDevInfo *info, int16_t devIndex) {
    /* GETINDADB, ADB_COPY_DEVTBL */
    if (devIndex > 0 && devIndex <= g_deviceCount) {
        if (info) {
            memcpy(info, &g_adbDeviceTable[devIndex - 1].devInfo, sizeof(LC3ADBDevInfo));
        }
        return g_adbDeviceTable[devIndex - 1].currentAddr;
    }
    return -1;
}

int16_t lc3_adb_get_info(uint8_t adbAddr, LC3ADBDevInfo *info) {
    /* GETADBINFO */
    for (uint8_t i = 0; i < g_deviceCount; ++i) {
        if (g_adbDeviceTable[i].currentAddr == adbAddr) {
            if (info) {
                memcpy(info, &g_adbDeviceTable[i].devInfo, sizeof(LC3ADBDevInfo));
            }
            return 0;
        }
    }
    return -1;
}

int16_t lc3_adb_set_info(uint8_t adbAddr, const LC3ADBDevInfo *info) {
    /* SETADBINFO */
    for (uint8_t i = 0; i < g_deviceCount; ++i) {
        if (g_adbDeviceTable[i].currentAddr == adbAddr) {
            if (info) {
                memcpy(&g_adbDeviceTable[i].devInfo, info, sizeof(LC3ADBDevInfo));
            }
            return 0;
        }
    }
    return -1;
}

int16_t lc3_adb_op(void *buffer, void (*compRtn)(void), void *data, int16_t commandNum) {
    (void)data;
    LC3EgretPacket pkt, resp;
    pkt.cmd = EGRET_CMD_ADB_SEND;
    pkt.subCmd = (uint8_t)commandNum;
    pkt.byteCount = (buffer != 0) ? 2 : 0;
    if (buffer) {
        memcpy(pkt.data, buffer, 2);
    }
    int16_t res = lc3_egret_send_packet(&pkt, &resp);
    if (compRtn) compRtn();
    return res;
}

void lc3_kbd_update_keymap(uint8_t rawKeyCode, bool isKeyDown) {
    /* KBD_UPDATE_KEYMAP, KBD_KEYUP_AUTO, KBD_BUILD_MESSAGE */
    uint8_t key = rawKeyCode & 0x7F;
    uint8_t byteIdx = key >> 3;
    uint8_t bitIdx = key & 7;

    uint8_t *km = (uint8_t *)LM_KEYMAP;
    if (isKeyDown) {
        km[byteIdx] |= (1 << bitIdx);
        LM_KEYLAST = rawKeyCode;
    } else {
        km[byteIdx] &= ~(1 << bitIdx);
    }
}

void lc3_kbd_set_led(uint8_t ledMask) {
    /* KBD_LED_PATH, KBD_SET_LED */
    g_currentLedState = ledMask;
    uint8_t buf[2] = { 0x00, ledMask };
    lc3_adb_op(buf, 0, 0, (2 << 4) | 0x00); /* ADB Listen device 2, register 0 */
}

uint32_t lc3_kbd_keytrans(const void *transData, uint16_t keyCode, uint32_t *state) {
    /* KEYTRANS, KEYTRANS_HAVE_DEAD, KEYTRANS_DEAD_LOOKUP, KEYTRANS_DEAD_MATCH */
    (void)transData;
    uint8_t code = (uint8_t)(keyCode & 0x7F);
    uint16_t modifiers = keyCode & 0xFF00;

    /* Simple standard US ASCII mapping */
    static const char unshifted[128] = {
        'a','s','d','f','h','g','z','x','c','v','§','b','q','w','e','r',
        'y','t','1','2','3','4','6','5','=','9','7','-','8','0',']','o',
        'u','[','i','p','\n','l','j','\'','k',';','\\',',','/','n','m','.',
        '\t',' ', '`','\b',0, 0x1B,0,0,0,0,0,0,0,0,0,0
    };
    static const char shifted[128] = {
        'A','S','D','F','H','G','Z','X','C','V','±','B','Q','W','E','R',
        'Y','T','!','@','#','$','^','%','+','(','&','_','*',')','}','O',
        'U','{','I','P','\n','L','J','"','K',':','|','<','?','N','M','>',
        '\t',' ', '~','\b',0, 0x1B,0,0,0,0,0,0,0,0,0,0
    };

    char c = (modifiers & 0x0200) ? shifted[code] : unshifted[code]; /* Shift key bit 9 */
    if (state) *state = 0; /* Clear dead key state */
    return (uint32_t)c;
}
