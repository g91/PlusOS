/* sysinfo - machine, hardware capabilities, video cards/drivers, memory and uptime. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

static const char *fpu_str(uint8_t f)
{
    switch (f) {
    case 1: return "68881";
    case 2: return "68882";
    case 3: return "68040 integrated";
    default: return "none";
    }
}

static const char *mmu_str(uint8_t m)
{
    switch (m) {
    case 1: return "AMU";
    case 2: return "68851 PMMU";
    case 3: return "68030 PMMU";
    case 4: return "68040 MMU";
    default: return "none";
    }
}

int main(void)
{
    struct plusos_sysinfo si;
    plusos_syscall(SYS_sysinfo, &si);
    unsigned long up = si.ticks / 60;
    printf("%s, %s, %luK RAM\n", si.machine, si.cpu, si.ram_bytes >> 10);

    struct plusos_sys7_args s7;
    memset(&s7, 0, sizeof s7);
    s7.op = PLUSOS_S7_HWINFO;
    if (plusos_syscall(SYS_sys7, &s7) == 0) {
        printf("BoxFlag $%02X, ROM v%04X (%luK), FPU: %s, MMU: %s\n",
               s7.box_flag, s7.rom_version, (unsigned long)(s7.rom_size >> 10),
               fpu_str(s7.fpu_type), mmu_str(s7.mmu_type));
        printf("Buses/HW: ADB=%s XPRAM=%s SlotMgr=%s ColorQD=%s ASC=%s 32bit=%s\n",
               s7.has_adb ? "yes" : "no", s7.has_xpram ? "yes" : "no",
               s7.has_slot_mgr ? "yes" : "no", s7.has_color_qd ? "yes" : "no",
               s7.has_asc ? "yes" : "no", s7.is_32bit_mode ? "32b" : "24b");
    }

    struct plusos_video_args va;
    memset(&va, 0, sizeof va);
    va.op = PLUSOS_VID_INFO;
    if (plusos_syscall(SYS_videoinfo, &va) == 0) {
        printf("Display: %ux%u @ %u bpp (rowBytes %u), %d video device(s):\n",
               va.width, va.height, va.depth, va.rowbytes, va.card_count);
        for (int i = 0; i < va.card_count; i++) {
            va.op = PLUSOS_VID_GET_CARD;
            va.index = i;
            if (plusos_syscall(SYS_videoinfo, &va) == 0) {
                printf("  [%d]%s slot $%X id=%u mode=%u %ux%ux%u drv=%s (%s)\n",
                       i, va.card.is_main ? "*" : " ", va.card.slot,
                       va.card.sp_id, va.card.mode_id,
                       va.card.width, va.card.height, va.card.depth,
                       va.card.driver_name[0] ? va.card.driver_name : "-",
                       va.card.card_name);
            }
        }
    }

    struct plusos_gestalt_args ga;
    ga.selector = 0x73797376UL; /* 'sysv' */
    if (plusos_syscall(SYS_gestalt, &ga) == 0)
        printf("Gestalt sysv: $%04lX (System 7.1 compat)\n", (unsigned long)ga.response);

    printf("boot volume \"%s\", %luK free in the ROM heap\n", si.volume, si.free_bytes >> 10);
    printf("up %lu:%02lu:%02lu, %d processes\n", up / 3600, (up / 60) % 60, up % 60, si.nprocs);
    if (si.is_se) {
        if (si.startup_id >= 0) printf("SCSI startup disk: ID %d\n", si.startup_id);
        else printf("SCSI startup disk: not set (highest ID boots)\n");
    }
    return 0;
}
