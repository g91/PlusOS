/* menu.c - the PlusOS multi-boot menu. Chooses which operating system to start:
 * another System on a SCSI disk (chain-boot through the ROM, Mac SE), or
 * PlusOS shell/desktop.
 *
 * Configurable via "multiboot.cfg" on the boot volume.
 */
#include "kernel.h"
#include "proc.h"
#include <plusos/syscall.h>

#define MAX_MENU_ITEMS 10

typedef struct {
    char label[32];
    int  scsi_id;   /* >= 0: SCSI ID, -1: Shell, -2: Desktop */
} MenuItem;

static MenuItem items[MAX_MENU_ITEMS];
static int nitems = 0;
static int cfg_timeout = 0;
static int cfg_default = -1;
static int cfg_direct_mac = -1;

static void set_default_items(void)
{
    nitems = 5;
    strlcpy(items[0].label, "System 7.5.5", sizeof(items[0].label)); items[0].scsi_id = 3;
    strlcpy(items[1].label, "System 7.1",   sizeof(items[1].label)); items[1].scsi_id = 2;
    strlcpy(items[2].label, "System 6.0.8", sizeof(items[2].label)); items[2].scsi_id = 1;
    strlcpy(items[3].label, "PlusOS Shell", sizeof(items[3].label)); items[3].scsi_id = -1;
    strlcpy(items[4].label, "PlusOS Desktop", sizeof(items[4].label)); items[4].scsi_id = -2;
}

static void trim(char *s)
{
    char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
}

static void parse_multiboot_cfg(const char *text)
{
    nitems = 0;
    char line[128];
    const char *p = text;
    char cur_name[32] = {0};
    int cur_id = -999;

    while (*p && nitems < MAX_MENU_ITEMS) {
        int idx = 0;
        while (*p && *p != '\n' && *p != '\r' && idx < (int)sizeof(line) - 1) {
            line[idx++] = *p++;
        }
        line[idx] = '\0';
        while (*p == '\r' || *p == '\n') p++;
        trim(line);

        if (!line[0] || line[0] == '#' || line[0] == ';') continue;

        if (line[0] == '[') {
            if (cur_name[0] && cur_id != -999 && nitems < MAX_MENU_ITEMS) {
                strlcpy(items[nitems].label, cur_name, sizeof(items[nitems].label));
                items[nitems].scsi_id = cur_id;
                nitems++;
            }
            cur_name[0] = '\0';
            cur_id = -999;
            continue;
        }

        /* Check key = value */
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char *k = line; trim(k);
            char *v = eq + 1; trim(v);
            if (!strcasecmp(k, "timeout")) {
                cfg_timeout = atoi(v);
            } else if (!strcasecmp(k, "default")) {
                cfg_default = atoi(v) - 1;
            } else if (!strcasecmp(k, "direct_mac")) {
                cfg_direct_mac = atoi(v);
            } else if (!strcasecmp(k, "name") || !strcasecmp(k, "title") || !strcasecmp(k, "label")) {
                strlcpy(cur_name, v, sizeof(cur_name));
            } else if (!strcasecmp(k, "id") || !strcasecmp(k, "scsi_id") || !strcasecmp(k, "target")) {
                if (!strcasecmp(v, "shell")) cur_id = -1;
                else if (!strcasecmp(v, "desktop")) cur_id = -2;
                else cur_id = atoi(v);
            }
            continue;
        }

        /* Check CSV format: "Name, ID" */
        char *comma = strchr(line, ',');
        if (comma) {
            *comma = '\0';
            char *nm = line; trim(nm);
            char *idstr = comma + 1; trim(idstr);
            int sid = atoi(idstr);
            if (!strcasecmp(idstr, "shell")) sid = -1;
            else if (!strcasecmp(idstr, "desktop")) sid = -2;
            if (nm[0] && nitems < MAX_MENU_ITEMS) {
                strlcpy(items[nitems].label, nm, sizeof(items[nitems].label));
                items[nitems].scsi_id = sid;
                nitems++;
            }
        }
    }

    if (cur_name[0] && cur_id != -999 && nitems < MAX_MENU_ITEMS) {
        strlcpy(items[nitems].label, cur_name, sizeof(items[nitems].label));
        items[nitems].scsi_id = cur_id;
        nitems++;
    }
}

static void load_config(void)
{
    nitems = 0;
    if (g_flags & PLUSOS_HAVE_FS) {
        static const unsigned char cfg_name[] = "\x0D" "multiboot.cfg";
        INTEGER ref;
        if (FSOpen(cfg_name, g_boot_vref, &ref) == 0) {
            char buf[1024];
            LONGINT count = (LONGINT)sizeof(buf) - 1;
            if (FSRead(ref, &count, buf) == 0 && count > 0) {
                buf[count] = '\0';
                parse_multiboot_cfg(buf);
            }
            FSClose(ref);
        }
    }
    if (nitems == 0) {
        set_default_items();
    }
}

static void draw(int sel)
{
    con_clear();
    con_goto(2, 1);
    con_set_invert(1);
    printf(" PlusOS MultiBoot %s ", PLUSOS_VERSION);
    con_set_invert(0);
    printf("  %s  %s  %luK", plusrom_machine_name(), cpu_name(), (unsigned long)(ram_bytes() >> 10));

    for (int i = 0; i < nitems; i++) {
        con_goto(4, 4 + i * 2);
        if (i == sel) con_set_invert(1);
        printf(" %d  %-18s ", i + 1, items[i].label);
        con_set_invert(0);
        if (items[i].scsi_id >= 0) {
            printf("  (SCSI ID %d)", items[i].scsi_id);
        } else if (items[i].scsi_id == -1) {
            printf("  (PlusOS Shell)");
        } else if (items[i].scsi_id == -2) {
            printf("  (PlusOS Desktop)");
        }
    }

    con_goto(4, 4 + nitems * 2 + 1);
    printf("1-%d, arrows + Return, or click an item", nitems);

    con_goto(4, 4 + nitems * 2 + 2);
    if (g_hw.has_xpram) {
        int def = chainboot_get_default();
        if (def >= 0) printf("Default startup: SCSI ID %d", def);
        else          printf("Default startup: none set (boots highest ID)");
    } else {
        printf("Mac Plus: boots the highest SCSI ID or floppy.");
    }
}

static void print_drive(int refnum, int drive)
{
    int id = (refnum >= 0xFFD8 && refnum <= 0xFFDF) ? 0xFFDF - refnum : -1;
    printf("refNum %04x (ID %d) drive %d   ", refnum & 0xFFFF, id, drive);
}

static void choose(int sel)
{
    if (sel < 0 || sel >= nitems) return;
    const MenuItem *it = &items[sel];

    if (it->scsi_id == -1) {                       /* PlusOS shell */
        con_clear();
        shell_main();                              /* never returns */
    }
    if (it->scsi_id == -2) {                       /* PlusOS desktop */
        con_clear();
        char *av[] = { "desktop", 0 };
        exec_spawn("desktop", 1, av);
        return;
    }

    con_goto(4, 4 + nitems * 2 + 4);
    if (!g_hw.has_xpram) {
        printf("Switching Systems from the menu needs XPRAM (SE or newer).");
        con_goto(4, 4 + nitems * 2 + 5);
        printf("On the Mac Plus, set this disk's SCSI ID highest. Any key...");
        kbd_get_click();
        return;
    }

    printf("Booting %s (SCSI ID %d)...", it->label, it->scsi_id);
    proc_sleep_ticks(30);
    int rc = chainboot(it->scsi_id);              /* on success: writes XPRAM, restarts */

    /* only reached on failure */
    con_goto(4, 4 + nitems * 2 + 5);
    if (rc == -2) {
        printf("No SCSI disk at ID %d. Drives found:", it->scsi_id);
        con_goto(4, 4 + nitems * 2 + 6);
        chainboot_each_drive(print_drive);
    } else {
        printf("Could not set the startup device (rc %d).", rc);
    }
    con_goto(4, 4 + nitems * 2 + 8);
    printf("Any key...");
    kbd_get_click();
}

void menu_main(void)
{
    load_config();

    /* Direct Mac OS boot option configured */
    if (cfg_direct_mac >= 0 && cfg_timeout <= 0 && g_hw.has_xpram) {
        chainboot(cfg_direct_mac);
    }

    int sel = 0;
    if (cfg_default >= 0 && cfg_default < nitems) {
        sel = cfg_default;
    } else {
        /* Default to shell or first PlusOS entry if found, otherwise first item */
        for (int i = 0; i < nitems; i++) {
            if (items[i].scsi_id < 0) { sel = i; break; }
        }
    }

    /* Auto-boot countdown timer if timeout configured */
    if (cfg_timeout > 0) {
        draw(sel);
        int remaining = cfg_timeout;
        while (remaining > 0) {
            con_goto(4, 4 + nitems * 2 + 4);
            printf("Auto-booting '%s' in %d second%s... (key cancels)   ",
                   items[sel].label, remaining, remaining == 1 ? "" : "s");
            for (int t = 0; t < 60; t++) {
                int k = kbd_poll();
                if (k || Button()) {
                    remaining = -1;
                    con_goto(4, 4 + nitems * 2 + 4);
                    printf("Auto-boot cancelled. Interactive menu ready.           ");
                    break;
                }
                proc_sleep_ticks(1);
            }
            if (remaining > 0) remaining--;
        }
        if (remaining == 0) {
            choose(sel);
        }
    }

    for (;;) {
        draw(sel);
        int k = kbd_get_click();
        if (k == KEY_CLICK) {
            int row = (int16_t)LM_WORD(0x830) / 8;  /* Mouse.v, 8-pixel text rows */
            int item = (row - 4) / 2;
            if (row >= 4 && (row - 4) % 2 == 0 && item >= 0 && item < nitems) {
                sel = item;
                choose(sel);
            }
            continue;
        }
        if (k >= '1' && k <= '0' + (nitems <= 9 ? nitems : 9)) {
            sel = k - '1';
            choose(sel);
        } else if (k == KEY_UP && sel > 0) {
            sel--;
        } else if (k == KEY_DOWN && sel < nitems - 1) {
            sel++;
        } else if (k == KEY_RETURN || k == KEY_ENTER) {
            choose(sel);
        }
    }
}
