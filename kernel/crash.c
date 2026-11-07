/* crash.c - report CPU exceptions / system errors and return to the shell. */
#include "kernel.h"
#include "proc.h"

extern uint16_t exc_vector, exc_sr, exc_syserr;
extern uint32_t exc_pc, exc_addr;
extern void exc_vec_2(void), exc_vec_3(void), exc_vec_4(void), exc_vec_5(void),
            exc_vec_6(void), exc_vec_7(void), exc_vec_8(void), exc_vec_11(void);
extern void syserr_handler(void);

static const char *vector_name(int v)
{
    switch (v) {
    case 2: return "BUS ERROR";
    case 3: return "ADDRESS ERROR";
    case 4: return "ILLEGAL INSTRUCTION";
    case 5: return "DIVIDE BY ZERO";
    case 6: return "CHK EXCEPTION";
    case 7: return "TRAPV EXCEPTION";
    case 8: return "PRIVILEGE VIOLATION";
    case 11: return "LINE-F INSTRUCTION";
    default: return "EXCEPTION";
    }
}

static const char *syserr_name(int e)
{
    switch (e) {
    case 1: return "bus error"; case 2: return "address error"; case 3: return "illegal instruction";
    case 12: return "unimplemented trap (call not in this ROM)";
    case 15: return "segment loader"; case 25: return "out of memory";
    case 26: return "bad program launch"; case 28: return "stack ran into heap";
    default: return "system error";
    }
}

void exc_install(void)
{
    LM_PTR(0x08) = (void *)exc_vec_2;
    LM_PTR(0x0C) = (void *)exc_vec_3;
    LM_PTR(0x10) = (void *)exc_vec_4;
    LM_PTR(0x14) = (void *)exc_vec_5;
    LM_PTR(0x18) = (void *)exc_vec_6;
    LM_PTR(0x1C) = (void *)exc_vec_7;
    LM_PTR(0x20) = (void *)exc_vec_8;
    LM_PTR(0x2C) = (void *)exc_vec_11;
    /* _SysError is Toolbox trap $A9C9. The Toolbox table is at $0C00 on the
     * Plus but $0E00 on the SE ($0C00 holds the SCSI globals there). */
    *plusrom_trap_slot(0xA9C9) = (void *)syserr_handler;
}

__attribute__((noreturn)) void kernel_crash(void)
{
    con_set_invert(0);
    con_set_cursor(0);
    if (g_flags & PLUSOS_HAVE_QD) {
        ObscureCursor();
        SetPort(&g_port);
    }
    printf("\n");
    con_set_invert(1);
    if (exc_vector == 0xFFFF)
        printf(" SYSTEM ERROR %d: %s ", exc_syserr, syserr_name(exc_syserr));
    else
        printf(" %s ", vector_name(exc_vector));
    con_set_invert(0);
    printf("\n  PC=%08lx  SR=%04x", (unsigned long)exc_pc, exc_sr);
    if (exc_vector == 2 || exc_vector == 3)
        printf("  ADDRESS=%08lx", (unsigned long)exc_addr);
    uint32_t base = __plusos_load_addr;
    if (exc_pc >= base && exc_pc < base + 0x30000)
        printf("  (kernel+%05lx)", (unsigned long)(exc_pc - base));
    {
        extern uint32_t exc_ssp, exc_usp;
        Proc *pc = proc_current;
        printf("\n  proc %d '%s' ring %d  SSP=%08lx USP=%08lx", pc ? pc->pid : -1, pc ? pc->name : "?",
               pc ? (int)pc->ring : -1, (unsigned long)exc_ssp, (unsigned long)exc_usp);
        if (pc && pc->kstack)
            printf("\n  kstack %08lx-%08lx ustack %08lx image %08lx", (unsigned long)pc->kstack,
                   (unsigned long)(pc->kstack + KSTACK_SIZE), (unsigned long)pc->ustack, (unsigned long)pc->image);
        uint32_t *sp = (uint32_t *)(exc_ssp & ~1u);
        printf("\n  stack:");
        for (int i = 0; i < 12; i++) printf(" %08lx", (unsigned long)sp[i]);
    }
    printf("\n  back to the shell.\n");
    shell_main();
}

/* exit()/abort() inside the kernel itself */
void exit(int code)
{
    printf("\nkernel exit(%d)\n", code);
    shell_main();
}

void abort(void)
{
    printf("\nkernel abort()\n");
    shell_main();
}
