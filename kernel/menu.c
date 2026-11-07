/* menu.c - the PlusOS boot menu. Chooses which operating system to start:
 * another System on a SCSI disk (chain-boot through the ROM, Mac SE), or the
 * PlusOS shell. */
#include "kernel.h"
#include "proc.h"

typedef struct { const char *label; int scsi_id; } MenuItem;

/* SCSI IDs match the BlueSCSI card: ID1 = 6.0.8, ID2 = 7.1, ID3 = 7.5.5,
 * ID6 = PlusOS. scsi_id < 0 means "run the PlusOS shell". */
static const MenuItem items[] = {
    { "System 7.5.5", 3 },
    { "System 7.1",   2 },
    { "System 6.0.8", 1 },
    { "PlusOS shell", -1 },
};
#define NITEMS 4

static void draw(int sel)
{
    con_clear();
    con_goto(2, 1);
    con_set_invert(1);
    printf(" PlusOS %s ", PLUSOS_VERSION);
    con_set_invert(0);
    printf("   %s  %s  %luK", plusrom_machine_name(), cpu_name(), (unsigned long)(ram_bytes() >> 10));
    for (int i = 0; i < NITEMS; i++) {
        con_goto(4, 4 + i * 2);
        if (i == sel) con_set_invert(1);
        printf(" %d  %-14s ", i + 1, items[i].label);
        con_set_invert(0);
        if (items[i].scsi_id >= 0) printf("  (SCSI ID %d)", items[i].scsi_id);
    }
    con_goto(4, 4 + NITEMS * 2 + 1);
    printf("1-4 or arrows + Return, or click an item");
    con_goto(4, 4 + NITEMS * 2 + 2);
    if (machine_is_se()) {
        int def = chainboot_get_default();
        if (def >= 0) printf("Default startup: SCSI ID %d", def);
        else          printf("Default startup: none set (boots highest ID)");
    } else {
        printf("Mac Plus: boots the highest SCSI ID; use the shell.");
    }
}

static void print_drive(int refnum, int drive)
{
    int id = (refnum >= 0xFFD8 && refnum <= 0xFFDF) ? 0xFFDF - refnum : -1;
    printf("refNum %04x (ID %d) drive %d   ", refnum & 0xFFFF, id, drive);
}

static void choose(int sel)
{
    const MenuItem *it = &items[sel];
    if (it->scsi_id < 0) {                       /* PlusOS shell */
        con_clear();
        shell_main();                            /* never returns */
    }
    con_goto(4, 4 + NITEMS * 2 + 4);
    if (!machine_is_se()) {
        printf("Switching Systems from the menu needs a Mac SE.");
        con_goto(4, 4 + NITEMS * 2 + 5);
        printf("On the Mac Plus, set this disk's SCSI ID highest. Any key...");
        kbd_get_click();
        return;
    }
    printf("Booting %s (SCSI ID %d)...", it->label, it->scsi_id);
    proc_sleep_ticks(30);
    int rc = chainboot(it->scsi_id);              /* on success: writes XPRAM, restarts */
    /* only reached on failure */
    con_goto(4, 4 + NITEMS * 2 + 5);
    if (rc == -2) {
        printf("No SCSI disk at ID %d. Drives found:", it->scsi_id);
        con_goto(4, 4 + NITEMS * 2 + 6);
        chainboot_each_drive(print_drive);
    } else {
        printf("Could not set the startup device (rc %d).", rc);
    }
    con_goto(4, 4 + NITEMS * 2 + 8);
    printf("Any key...");
    kbd_get_click();
}

void menu_main(void)
{
    int sel = NITEMS - 1;                          /* default: PlusOS shell */
    for (;;) {
        draw(sel);
        int k = kbd_get_click();
        if (k == KEY_CLICK) {
            int row = (int16_t)LM_WORD(0x830) / 8;  /* Mouse.v, 8-pixel text rows */
            int item = (row - 4) / 2;
            if (row >= 4 && (row - 4) % 2 == 0 && item >= 0 && item < NITEMS) {
                sel = item;
                choose(sel);
            }
            continue;
        }
        if (k >= '1' && k <= '0' + NITEMS) { sel = k - '1'; choose(sel); }
        else if (k == KEY_UP && sel > 0) sel--;
        else if (k == KEY_DOWN && sel < NITEMS - 1) sel++;
        else if (k == KEY_RETURN || k == KEY_ENTER) choose(sel);
    }
}
