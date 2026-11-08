/* trap.S - the TRAP #1 syscall entry. The CPU has already switched to
 * supervisor and pushed the caller's SR+PC onto the kernel stack. We call
 * syscall_dispatch(num, arg) and RTE back with the result in D0.
 *
 * Every register except D0 is preserved: C may clobber d1/a0/a1, and the
 * caller must not see that (it may hold live values there across the trap). */
        .text
        .globl  trap1_entry
trap1_entry:
        movem.l %d1/%a0-%a1, -(%sp)
        move.l  %a0, -(%sp)                 /* arg block pointer */
        move.l  %d0, -(%sp)                 /* syscall number */
        jsr     syscall_dispatch            /* long syscall_dispatch(num, arg) -> D0 */
        addq.l  #8, %sp
        movem.l (%sp)+, %d1/%a0-%a1
        rte                                 /* back to caller (user or supervisor), D0 = result */
