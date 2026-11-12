/* shell.c - the PlusOS command shell. Anything that is not a built-in command
 * is run as a program from the disk. */
#include "kernel.h"
#include "proc.h"
#include "kconfig.h"
#include "users.h"
#include "environ.h"

#define MAX_ARGS 16

typedef struct {
    const char *name;
    void (*fn)(int argc, char **argv);
    const char *help;
} Command;

static void cmd_help(int, char **);

/* ---------------------------------------------------------------- commands */
static void cmd_cls(int argc, char **argv)  { con_clear(); }
static void cmd_ver(int argc, char **argv)
{
    printf("PlusOS %s by the1Domo on the %s - %s, %luK RAM\n", PLUSOS_VERSION, hw_machine_name(), cpu_name(),
           (unsigned long)(ram_bytes() >> 10));
}
static void cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) printf("%s%s", argv[i], i + 1 < argc ? " " : "");
    printf("\n");
}
static void cmd_menu(int argc, char **argv) { menu_main(); }

static void drives_cb(int refnum, int drive)
{
    int id = (refnum >= 0xFFD8 && refnum <= 0xFFDF) ? 0xFFDF - refnum : -1;
    printf("  drive %2d  refNum %04x  %s%d\n", drive, refnum & 0xFFFF,
           id >= 0 ? "SCSI ID " : "(not SCSI)", id);
}

static void cmd_drives(int argc, char **argv)
{
    printf("Drive queue:\n");
    chainboot_each_drive(drives_cb);
    if (g_hw.has_xpram) {
        int d = chainboot_get_default();
        if (d >= 0) printf("default startup: SCSI ID %d\n", d);
        else printf("default startup: none (boots highest ID)\n");
    }
}

static void cmd_boot(int argc, char **argv)
{
    if (!g_hw.has_xpram) {
        printf("boot: switching Systems needs XPRAM (SE or newer); the Plus boots the highest SCSI ID.\n");
        return;
    }
    if (argc < 2) {
        int def = chainboot_get_default();
        if (def >= 0) printf("default startup: SCSI ID %d\n", def);
        else printf("default startup: none (boots highest ID)\n");
        printf("usage: boot <scsi-id>   (e.g. boot 3 for System 7.5.5)\n");
        return;
    }
    int id = atoi(argv[1]);
    printf("Setting startup to SCSI ID %d and restarting...\n", id);
    proc_sleep_ticks(30);
    chainboot(id);
    printf("boot: could not set SCSI ID %d\n", id);
}
static void cmd_reboot(int argc, char **argv) { sys_reboot(); }
static void cmd_off(int argc, char **argv) { sys_shutdown(); }
static void cmd_beep(int argc, char **argv) { SysBeep(argc > 1 ? atoi(argv[1]) : 10); }
static void cmd_gui(int argc, char **argv) { gfx_demo(); }
static void cmd_mouse(int argc, char **argv) { gfx_mouse(); }
static void cmd_paint(int argc, char **argv) { gfx_paint(); }

static void cmd_mem(int argc, char **argv)
{
    printf("RAM            %luK  (MemTop %06lx)\n", (unsigned long)(ram_bytes() >> 10),
           (unsigned long)ram_bytes());
    printf("kernel         %06lx  stack %06lx-%06lx\n", (unsigned long)__plusos_load_addr,
           (unsigned long)(__plusos_load_addr - KERNEL_STACK), (unsigned long)__plusos_load_addr);
    printf("system heap    %06lx\n", (unsigned long)LM_LONG(0x2A6));
    if (g_flags & PLUSOS_HAVE_HEAP) {
        printf("app heap       %06lx-%06lx\n", (unsigned long)LM_LONG(0x2AA), (unsigned long)LM_LONG(0x130));
        printf("free           %luK (largest block %luK)\n", (unsigned long)(FreeMem() >> 10),
               (unsigned long)(MaxBlock() >> 10));
    }
    printf("screen         %06lx\n", (unsigned long)LM_LONG(0x824));
}

extern volatile uint32_t heartbeat;

static void cmd_uptime(int argc, char **argv)
{
    uint32_t t = tick_count();
    printf("up %lu.%02lu s  (%lu ticks)  heartbeat %lu  processes %d\n",
           (unsigned long)(t / 60), (unsigned long)((t % 60) * 100 / 60),
           (unsigned long)t, (unsigned long)heartbeat, proc_count());
}

static void cmd_lsmod(int argc, char **argv)
{
    printf("MODULE    TYPE     STAGE     STATE    INIT\n");
    for (int i = 0; i < kmod_count(); i++) {
        kmodule *m = kmod_at(i);
        printf("%-9s %-8s %-9s %-8s %ut%s\n", m->name, kmod_type_name(m->type),
               kmod_stage_name(m->stage), kmod_state_name(m->state), (unsigned)m->init_ticks,
               (m->flags & KMOD_F_ESSENTIAL) ? "  essential" : "");
    }
    printf("drivers:");
    for (int i = 0; i < kdrv_count(); i++) printf(" %s", kdrv_at(i)->name);
    printf("\nstack guard %s, watchdog %s (stalls %lu, worst %lu ticks)\n",
           CONFIG_STACKGUARD ? "on" : "off", CONFIG_WATCHDOG ? "on" : "off",
           (unsigned long)kwd_stalls(), (unsigned long)kwd_worst());
}

static void cmd_ps(int argc, char **argv)
{
    static const char *st[] = { "unused", "ready", "run", "blocked", "zombie" };
    printf("PID RING STATE    NAME\n");
    for (int i = 0; i < MAX_PROC; i++) {
        Proc *p = proc_table_entry(i);
        if (p->state == PROC_UNUSED) continue;
        printf("%3d  %s  %-7s  %s%s\n", p->pid, p->ring == RING_USER ? "u" : "k",
               st[p->state], p->name, p == proc_current ? " *" : "");
    }
}

static void cmd_time(int argc, char **argv)
{
    ULONGINT secs;
    ReadDateTime(&secs);
    DateTimeRec d;
    SecondsToDate(secs, &d);
    static const char *days[] = { "", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    printf("%04d-%02d-%02d %02d:%02d:%02d  %s\n", d.year, d.month, d.day, d.hour, d.minute, d.second,
           (d.dayOfWeek >= 1 && d.dayOfWeek <= 7) ? days[d.dayOfWeek] : "");
}

static void cmd_rom(int argc, char **argv)
{
    uint8_t *rom = (uint8_t *)LM_LONG(0x2AE);
    printf("ROM at %06lx  checksum %08lx  version %04x\n", (unsigned long)rom,
           (unsigned long)*(uint32_t *)rom, *(uint16_t *)(rom + 8));
    int se = machine_is_se();
    printf("%s: %d traps in ROM (Mac Plus %d, Mac SE %d)\n", plusrom_machine_name(),
           se ? SEROM_TOOLBOX_TRAPS + SEROM_OS_TRAPS : PLUSROM_TOOLBOX_TRAPS + PLUSROM_OS_TRAPS,
           PLUSROM_TOOLBOX_TRAPS + PLUSROM_OS_TRAPS, SEROM_TOOLBOX_TRAPS + SEROM_OS_TRAPS);
    printf("Toolbox trap table at %04lx\n", (unsigned long)(uintptr_t)plusrom_tool_table());
    printf("managers (calls in this ROM / total):\n");
    int col = 0;
    /* both catalogs list the managers in the same order */
    for (unsigned m = 0; m < plusrom_manager_count; m++) {
        const char *name = se ? serom_managers[m].name : plusrom_managers[m].name;
        unsigned total = se ? *serom_managers[m].count : *plusrom_managers[m].count;
        int in = 0;
        for (unsigned i = 0; i < total; i++)
            in += se ? serom_managers[m].calls[i].in_rom : plusrom_managers[m].calls[i].in_rom;
        if (!in) continue;
        printf("  %-16s %3d of %-4u", name, in, total);
        if (++col == 2) { col = 0; printf("\n"); }
    }
    if (col) printf("\n");
}

static uint32_t trap_handler(uint16_t t) { return (uint32_t)(uintptr_t)plusrom_trap_address(t); }

static void cmd_trap(int argc, char **argv)
{
    if (argc < 2) { printf("usage: trap NAME | trap A8A1\n"); return; }
    char *end;
    unsigned long v = strtoul(argv[1], &end, 16);
    if (*end == 0 && (v & 0xF000) == 0xA000) {
        uint16_t t = (uint16_t)v;
        int here = machine_is_se() ? serom_trap_in_rom(t) : plusrom_trap_in_rom(t);
        printf("trap %04x: Mac Plus %s, Mac SE %s", t, plusrom_trap_in_rom(t) ? "yes" : "no",
               serom_trap_in_rom(t) ? "yes" : "no");
        if (here) printf("; handler here at %06lx", (unsigned long)trap_handler(t));
        printf("\n");
        return;
    }
    const PlusRomManager *pm;
    const PlusRomCall *pc = plusrom_find_call(argv[1], &pm);      /* Mac Plus SDK catalog */
    const SeRomCall *sc = serom_find_call(argv[1], 0);           /* Mac SE SDK catalog */
    if (!pc) { printf("unknown call '%s'\n", argv[1]); return; }
    printf("%s (%s.h)\n", pc->name, pm->name);
    if (pc->trap) printf("  trap %04x", pc->trap);
    if (pc->selector >= 0) printf("  selector %ld", (long)pc->selector);
    printf("\n  Mac Plus: %s", pc->in_rom ? "yes" : "no");
    if (pc->in_rom && pc->trap) printf(" (ROM offset %05lx)", (unsigned long)pc->rom_offset);
    printf("\n  Mac SE:   %s", sc && sc->in_rom ? "yes" : "no");
    if (sc && sc->in_rom && sc->trap) printf(" (ROM offset %05lx)", (unsigned long)sc->rom_offset);
    int here = machine_is_se() ? (sc && sc->in_rom) : pc->in_rom;
    if (here && pc->trap)
        printf("\n  handler on this %s: %06lx", plusrom_machine_name(), (unsigned long)trap_handler(pc->trap));
    printf("\n");
}

static void cmd_peek(int argc, char **argv)
{
    if (argc < 2) { printf("usage: peek ADDRESS [BYTES]\n"); return; }
    uint8_t *p = (uint8_t *)strtoul(argv[1], 0, 16);
    int n = argc > 2 ? (int)strtoul(argv[2], 0, 0) : 64;
    for (int i = 0; i < n; i += 16) {
        printf("%06lx ", (unsigned long)(p + i));
        for (int j = 0; j < 16; j++) printf(i + j < n ? "%02x" : "  ", p[i + j]), printf(j == 7 ? "  " : " ");
        for (int j = 0; j < 16 && i + j < n; j++) putchar(isprint(p[i + j]) ? p[i + j] : '.');
        printf("\n");
    }
}

static void cmd_poke(int argc, char **argv)
{
    if (argc < 3) { printf("usage: poke ADDRESS BYTE...\n"); return; }
    uint8_t *p = (uint8_t *)strtoul(argv[1], 0, 16);
    for (int i = 2; i < argc; i++) *p++ = (uint8_t)strtoul(argv[i], 0, 16);
}

static int need_fs(void)
{
    if (g_flags & PLUSOS_HAVE_FS) return 1;
    printf("no file system (the boot volume is not mounted)\n");
    return 0;
}

static void cmd_ls(int argc, char **argv)
{
    if (!need_fs()) return;
    printf("Volume \"%s\"\n", g_volname);
    long files = 0, total = 0;
    for (int i = 1; ; i++) {
        CInfoPBRec pb;
        Str255 name;
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = name;
        pb.hFileInfo.ioVRefNum = g_boot_vref;
        pb.hFileInfo.ioFDirIndex = (INTEGER)i;
        pb.hFileInfo.ioDirID = 2;                         /* root directory */
        if (PBGetCatInfoSync(&pb) != 0) break;
        char cname[64];
        p2cstr_buf(cname, name);
        if (pb.hFileInfo.ioFlAttrib & 0x10) {
            printf("  %-31s  <folder>\n", cname);
        } else {
            uint32_t type = (uint32_t)pb.hFileInfo.ioFlFndrInfo.fdType;
            long size = pb.hFileInfo.ioFlLgLen + pb.hFileInfo.ioFlRLgLen;
            printf("  %-31s %8ld  %c%c%c%c\n", cname, size, (char)(type >> 24), (char)(type >> 16),
                   (char)(type >> 8), (char)type);
            files++;
            total += size;
        }
    }
    printf("%ld files, %ld bytes\n", files, total);
}

static void cmd_cat(int argc, char **argv)
{
    if (argc < 2) { printf("usage: cat FILE\n"); return; }
    if (!need_fs()) return;
    char hfs_path[128];
#if CONFIG_USERS
    /* Unix path -> HFS colon path; "~" expands to the current user's home. */
    hfs_path_translate(argv[1], hfs_path, sizeof hfs_path);
#else
    strncpy(hfs_path, argv[1], sizeof hfs_path - 1); hfs_path[sizeof hfs_path - 1] = 0;
#endif
    Str255 pname;
    c2pstr_buf(pname, hfs_path);
    INTEGER ref;
    if (FSOpen(pname, g_boot_vref, &ref)) { printf("cannot open %s\n", argv[1]); return; }
    char buf[256];
    for (;;) {
        LONGINT n = sizeof buf;
        OSErr e = FSRead(ref, &n, buf);
        for (LONGINT i = 0; i < n; i++) putchar(buf[i] == '\r' ? '\n' : buf[i]);
        if (e || n == 0) break;
    }
    FSClose(ref);
    printf("\n");
}

static void cmd_keys(int argc, char **argv)
{
    printf("Raw keyboard test: every key event the ROM receives is listed,\n"
           "even keys PlusOS can't translate. Click the mouse to stop.\n");
    while (kbd_raw_next() >= 0) {}                  /* drop old events */
    while (kbd_poll()) {}
    for (;;) {
        int r;
        while ((r = kbd_raw_next()) >= 0) {
            int code = r & 0x7F;
            if (r & 0x80) printf("  key %02x DOWN", code);
            else          printf("  key %02x up\n", code);
        }
        int c;
        while ((c = kbd_poll()))                    /* the character, if any */
            printf("  -> char %02x '%c'", c, isprint(c) ? c : ' ');
        if (Button()) { while (Button()) {} break; }
        char s[80];
        kbd_status(s, sizeof s);
        con_status(s);
    }
    printf("\n");
}

static void run_program(int argc, char **argv)
{
    int pid = exec_spawn(argv[0], argc, argv);      /* ring-2 (user mode) process */
    if (pid <= -1001 && pid >= -1004) {
        printf("%s: %s\n", argv[0], exec_error(pid));
        return;
    }
    if (pid < 0) { printf("%s: cannot start (%d)\n", argv[0], pid); return; }
    int code = proc_wait(pid);                       /* block this task until it exits */
    if (code) printf("[%s exited with %d]\n", argv[0], code);
}

static void cmd_run(int argc, char **argv)
{
    if (argc < 2) { printf("usage: run PROGRAM [ARGS]\n"); return; }
    run_program(argc - 1, argv + 1);
}

static void cmd_history(int argc, char **argv)
{
    if (argc >= 2 && (!strcasecmp(argv[1], "-c") || !strcasecmp(argv[1], "clear"))) {
        kbd_history_clear();
        printf("command history cleared.\n");
        return;
    }
    int n = kbd_history_count();
    if (!n) { printf("(history is empty)\n"); return; }
    for (int i = 0; i < n; i++) {
        const char *h = kbd_history_get(i);
        if (h) printf("  %2d  %s\n", i + 1, h);
    }
}

/* NOTE: whoami, id, users, passwd, su are userland programs in /bin
 * (apps/tools/<name>/). The kernel recovery shell below is only reached when
 * the real userland shell cannot start, so those commands do not need to be
 * duplicated here. */

static const Command commands[] = {
    { "help",    cmd_help,    "this list" },
    { "history", cmd_history, "show last 20 commands (saved across reboots; -c clears)" },
    { "ls",      cmd_ls,      "list files on the boot volume (also: dir)" },
    { "dir",    cmd_ls,     0 },
    { "cat",    cmd_cat,    "show a text file (also: type)" },
    { "type",   cmd_cat,    0 },
    { "run",    cmd_run,    "run a program (or just type its name)" },
    { "ps",     cmd_ps,     "list processes" },
    { "uptime", cmd_uptime, "time since boot and the heartbeat task" },
    { "lsmod",  cmd_lsmod,  "kernel modules, drivers, watchdog and stack guard" },
    { "mem",    cmd_mem,    "memory map and free heap" },
    { "rom",    cmd_rom,    "ROM version and the managers in it" },
    { "trap",   cmd_trap,   "look up a ROM call: trap FrameRect / trap A8A1" },
    { "time",   cmd_time,   "date and time from the real-time clock (also: date)" },
    { "date",   cmd_time,   0 },
    { "gui",    cmd_gui,    "QuickDraw demo" },
    { "paint",  cmd_paint,  "draw with the mouse" },
    { "mouse",  cmd_mouse,  "show mouse position" },
    { "keys",   cmd_keys,   "keyboard test" },
    { "beep",   cmd_beep,   "SysBeep" },
    { "peek",   cmd_peek,   "hex dump memory: peek ADDR [BYTES]" },
    { "poke",   cmd_poke,   "write bytes: poke ADDR BYTE..." },
    { "echo",   cmd_echo,   "print the arguments" },
    { "cls",    cmd_cls,    "clear the screen (also: clear)" },
    { "clear",  cmd_cls,    0 },
    { "ver",    cmd_ver,    "version" },
    { "menu",   cmd_menu,   "the boot menu (choose an OS)" },
    { "boot",   cmd_boot,   "boot another System disk: boot <scsi-id> (Mac SE)" },
    { "drives", cmd_drives, "list the SCSI drive queue" },
    { "reboot", cmd_reboot, "restart the Mac" },
    { "off",    cmd_off,    "flush the disk and halt (also: shutdown)" },
    { "shutdown", cmd_off,  0 },
};
#define NCOMMANDS (sizeof commands / sizeof commands[0])

static void cmd_help(int argc, char **argv)
{
    for (unsigned i = 0; i < NCOMMANDS; i++)
        if (commands[i].help)
            printf("  %-8s %s\n", commands[i].name, commands[i].help);
}

static void execute(char *line)
{
    char *argv[MAX_ARGS + 1];
    int argc = 0;
    char *save;
    for (char *t = strtok_r(line, " \t", &save); t && argc < MAX_ARGS; t = strtok_r(0, " \t", &save))
        argv[argc++] = t;
    argv[argc] = 0;
    if (!argc) return;
    for (unsigned i = 0; i < NCOMMANDS; i++)
        if (!strcasecmp(argv[0], commands[i].name)) { commands[i].fn(argc, argv); return; }
    run_program(argc, argv);
}

void shell_main(void)
{
    static char line[128];
    static int banner;
    if (!banner++) {
        printf("\nPlusOS recovery console by the1Domo (built into the kernel). Type 'help'.\n");
    }
    for (;;) {
        con_set_invert(0);
        printf("recovery> ");
        kbd_line(line, sizeof line);
        execute(line);
    }
}
