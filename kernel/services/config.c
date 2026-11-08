/* config.c - persistent PlusOS settings.
 *
 * Stored in a small file, "PlusOS Settings", on the boot volume (written with
 * the ROM File Manager, so it survives restarts and can be seen from a Mac).
 * Without a file system the settings still work, but only until restart.
 */
#include "kernel.h"
#include <plusos/syscall.h>

#define CFG_MAGIC 0x504C4346u            /* 'PLCF' */

typedef struct {
    uint32_t magic;
    int16_t  version;
    int16_t  boot;                       /* PLUSOS_BOOT_SHELL / _DESKTOP */
    int16_t  reserved[12];
} Config;

static Config cfg = { CFG_MAGIC, 1, PLUSOS_BOOT_SHELL, { 0 } };
static int    loaded;
static const unsigned char cfg_name[] = "\x0F" "PlusOS Settings";

static void cfg_load(void)
{
    if (loaded) return;
    loaded = 1;
    if (!(g_flags & PLUSOS_HAVE_FS)) return;
    INTEGER ref;
    if (FSOpen(cfg_name, g_boot_vref, &ref) != 0) return;
    Config c;
    LONGINT n = sizeof c;
    if (FSRead(ref, &n, &c) == 0 && n == (LONGINT)sizeof c && c.magic == CFG_MAGIC)
        cfg = c;
    FSClose(ref);
}

static int cfg_save(void)
{
    if (!(g_flags & PLUSOS_HAVE_FS)) return -1;
    INTEGER ref;
    if (FSOpen(cfg_name, g_boot_vref, &ref) != 0) {
        Create(cfg_name, g_boot_vref, 0x504C4F53 /* 'PLOS' */, 0x70726566 /* 'pref' */);
        if (FSOpen(cfg_name, g_boot_vref, &ref) != 0) return -1;
    }
    LONGINT n = sizeof cfg;
    int rc = (SetFPos(ref, 1, 0) == 0 && FSWrite(ref, &n, &cfg) == 0 && n == (LONGINT)sizeof cfg) ? 0 : -1;
    SetEOF(ref, sizeof cfg);
    FSClose(ref);
    FlushVol(0, g_boot_vref);            /* make it stick even if power is cut */
    return rc;
}

int cfg_get(int key)
{
    cfg_load();
    switch (key) {
    case PLUSOS_CFG_BOOT: return cfg.boot;
    default:              return -1;
    }
}

int cfg_set(int key, int value)
{
    cfg_load();
    switch (key) {
    case PLUSOS_CFG_BOOT:
        if (value != PLUSOS_BOOT_SHELL && value != PLUSOS_BOOT_DESKTOP) return -1;
        cfg.boot = (int16_t)value;
        break;
    default:
        return -1;
    }
    return cfg_save();
}
