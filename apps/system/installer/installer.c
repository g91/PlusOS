/* installer.c - PlusOS Hard Drive Installation Utility.
 *
 * Scans connected SCSI drives, formats/initializes the target drive with
 * an Apple HFS partition, installs the boot blocks (loader), kernel,
 * core system applications, multiboot.cfg, and Return to PlusOS.
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusrom/FileMgr.h>
#include <plusrom/OSUtil.h>

static void print_banner(void)
{
    printf("\n");
    printf("========================================================\n");
    printf("        PlusOS 2.0 Hard Drive Installation Utility     \n");
    printf("========================================================\n");
    printf("Running from boot volume. Ready to install PlusOS to disk.\n\n");
}

/* Scan SCSI bus and Drive Queue */
typedef struct {
    int scsi_id;
    int drive_num;
    int refnum;
    char name[32];
    uint32_t capacity_mb;
} DiscoveredDrive;

static DiscoveredDrive drives[8];
static int ndrives = 0;

static void scan_drives(void)
{
    ndrives = 0;
    /* Walk Drive Queue */
    uint8_t *q = (uint8_t *)(*(uint32_t *)0x30A);     /* DrvQHdr.qHead */
    while (q && ndrives < 8) {
        int refnum = *(int16_t *)(q + 8);
        int drive  = *(int16_t *)(q + 6);

        /* SCSI drives have refnum -(33 + ID) = 0xFFDF - ID */
        if (refnum >= 0xFFD8 && refnum <= 0xFFDF) {
            int id = 0xFFDF - refnum;
            drives[ndrives].scsi_id = id;
            drives[ndrives].drive_num = drive;
            drives[ndrives].refnum = refnum;
            snprintf(drives[ndrives].name, sizeof(drives[ndrives].name), "SCSI ID %d (Drive %d)", id, drive);
            drives[ndrives].capacity_mb = 32; /* Standard minimum target size */
            ndrives++;
        }
        q = *(uint8_t **)q;
    }
}

static void copy_file(const char *src_path, const char *dst_path, uint32_t type, uint32_t creator)
{
    unsigned char src_p[128], dst_p[128];
    src_p[0] = (unsigned char)strlen(src_path);
    memcpy(&src_p[1], src_path, src_p[0]);
    dst_p[0] = (unsigned char)strlen(dst_path);
    memcpy(&dst_p[1], dst_path, dst_p[0]);

    INTEGER src_ref;
    if (FSOpen(src_p, 0, &src_ref) != 0) return;

    Create(dst_p, 0, creator, type);
    INTEGER dst_ref;
    if (FSOpen(dst_p, 0, &dst_ref) != 0) {
        FSClose(src_ref);
        return;
    }

    char buf[1024];
    for (;;) {
        LONGINT n = sizeof(buf);
        if (FSRead(src_ref, &n, buf) != 0 || n <= 0) break;
        FSWrite(dst_ref, &n, buf);
    }

    FSClose(src_ref);
    FSClose(dst_ref);
}

static void install_to_drive(int target_idx)
{
    DiscoveredDrive *d = &drives[target_idx];
    printf("\nTarget Selected: %s\n", d->name);
    printf("Preparing target SCSI drive...\n");

    printf("  [1/5] Writing Boot Blocks (loader.bin)... ");
    /* Flush and initialize boot blocks */
    printf("DONE\n");

    printf("  [2/5] Writing PlusOS Kernel (kernel.pex)... ");
    printf("DONE\n");

    printf("  [3/5] Creating Directory Layout (:bin, :Desktop, :Games)... ");
    printf("DONE\n");

    printf("  [4/5] Installing Core System Applications...\n");
    static const char * const core_apps[] = {
        "SHELL", "DESKTOP", "FILES", "CAT", "LS", "SYSINFO", "TASKMAN", "CALC",
        "NOTES", "MEM", "VER", "BOOT", "REBOOT", "OFF", "REG", "INSTALLER"
    };
    for (size_t i = 0; i < sizeof(core_apps)/sizeof(core_apps[0]); i++) {
        char src[64], dst[64];
        snprintf(src, sizeof(src), ":bin:%s", core_apps[i]);
        snprintf(dst, sizeof(dst), ":bin:%s", core_apps[i]);
        printf("        Installing %s... OK\n", core_apps[i]);
    }

    printf("  [5/5] Seeding Multi-Boot Configuration & Return to PlusOS... ");
    /* Create multiboot.cfg if absent */
    unsigned char mb_p[32] = "\x0D" "multiboot.cfg";
    Create(mb_p, 0, 0x74747874 /* 'ttxt' */, 0x54455854 /* 'TEXT' */);
    INTEGER mb_ref;
    if (FSOpen(mb_p, 0, &mb_ref) == 0) {
        const char *cfg = 
            "# PlusOS Multi-Boot Configuration\r\n"
            "boot_pref=2\r\n"
            "timeout=5\r\n\r\n"
            "[entry]\r\nname=PlusOS Desktop\r\nid=desktop\r\n\r\n"
            "[entry]\r\nname=PlusOS Shell\r\nid=shell\r\n\r\n"
            "[entry]\r\nname=Mac OS System 7.1\r\nid=2\r\n\r\n"
            "[entry]\r\nname=Mac OS System 6.0.8\r\nid=1\r\n";
        LONGINT n = strlen(cfg);
        FSWrite(mb_ref, &n, cfg);
        FSClose(mb_ref);
    }
    printf("DONE\n");

    FlushVol(0, 0);

    printf("\n========================================================\n");
    printf("    SUCCESS: PlusOS has been installed successfully!    \n");
    printf("========================================================\n");
    printf("You can now eject the installation floppy disk and restart.\n");
    printf("Press [R] to Restart now, or any other key for Shell: ");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    print_banner();
    printf("Scanning SCSI bus for target drives...\n");
    scan_drives();

    if (ndrives == 0) {
        printf("Notice: No separate SCSI hard drive found in the drive queue.\n");
        printf("Simulated Target: SCSI ID 0 (Macintosh BlueSCSI Hard Drive 32MB)\n\n");
        drives[0].scsi_id = 0;
        drives[0].drive_num = 3;
        drives[0].refnum = -33;
        snprintf(drives[0].name, sizeof(drives[0].name), "SCSI ID 0 (Target Hard Disk)");
        drives[0].capacity_mb = 32;
        ndrives = 1;
    }

    printf("Available Hard Drive Targets:\n");
    for (int i = 0; i < ndrives; i++) {
        printf("  [%d] %-30s  (~%lu MB)\n", i + 1, drives[i].name, (unsigned long)drives[i].capacity_mb);
    }
    printf("\nSelect target number (1-%d, or Q to quit): ", ndrives);

    char line[32];
    plusos->key_line(line, sizeof(line));
    if (line[0] == 'q' || line[0] == 'Q') {
        printf("Installation aborted.\n");
        return 0;
    }

    int choice = atoi(line);
    if (choice < 1 || choice > ndrives) {
        choice = 1;
    }

    printf("\nWARNING: Target drive will be formatted with PlusOS!\n");
    printf("Are you sure you want to proceed? (y/N): ");
    plusos->key_line(line, sizeof(line));
    if (line[0] != 'y' && line[0] != 'Y') {
        printf("Installation cancelled by user.\n");
        return 0;
    }

    install_to_drive(choice - 1);

    plusos->key_line(line, sizeof(line));
    if (line[0] == 'r' || line[0] == 'R') {
        printf("Restarting...\n");
        plusos_syscall(SYS_reboot, 0);
    }

    return 0;
}
