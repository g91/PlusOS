/* signal.c - PlusOS Signal Dispatching Subsystem */
#include "kernel.h"
#include "proc.h"
#include "signal.h"

void signal_init(void)
{
}

int signal_send(int pid, int signum)
{
    if (pid <= 0) return -1;
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_PROC; i++) {
        Proc *p = proc_table_entry(i);
        if (p && p->pid == pid && p->state != PROC_UNUSED && p->state != PROC_ZOMBIE) {
            p->sig_pending |= (1UL << signum);
            if (signum == PLUSOS_SIGKILL || signum == PLUSOS_SIGTERM || signum == PLUSOS_SIGINT) {
                p->killed = 1;
            }
            if (p->state == PROC_BLOCKED) {
                p->wait_chan = 0;
                p->wake_tick = 0;
                /* Wake to handle signal */
                p->state = PROC_READY;
            }
            int_restore(sr);
            return 0;
        }
    }
    int_restore(sr);
    return -1;
}

void signal_check(void)
{
    if (!proc_current || proc_current->ring != RING_USER) return;
    if (proc_current->sig_pending & (1UL << PLUSOS_SIGKILL)) {
        proc_exit(-9);
    }
    if (proc_current->sig_pending & (1UL << PLUSOS_SIGTERM)) {
        proc_exit(-15);
    }
    if (proc_current->sig_pending & (1UL << PLUSOS_SIGINT)) {
        proc_exit(-2);
    }
    /* Clear handled alarm or user signals */
    proc_current->sig_pending &= ~((1UL << PLUSOS_SIGALRM) | (1UL << PLUSOS_SIGUSR1) | (1UL << PLUSOS_SIGUSR2));
}
