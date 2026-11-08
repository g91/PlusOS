/* ram - PlusOS Dynamic RAM & Memory Management Monitor */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    struct plusos_ram_stats st;
    memset(&st, 0, sizeof st);

    if (plusos_syscall(SYS_ramstats, &st) != 0) {
        printf("ram: error retrieving RAM statistics.\n");
        return 1;
    }

    printf("================ PlusOS RAM Management ================\n");
    printf("Physical RAM:       %lu KB  (%lu bytes)\n",
           (unsigned long)(st.total_ram >> 10), (unsigned long)st.total_ram);
    printf("ROM Heap Free:      %lu KB  (largest block: %lu KB)\n",
           (unsigned long)(st.free_ram >> 10), (unsigned long)(st.largest_free_block >> 10));
    printf("Kernel Load Base:   $%06lx  (Kernel Stack: %lu KB)\n",
           (unsigned long)st.kernel_load_addr, (unsigned long)(st.kernel_stack_size >> 10));

    printf("\n--- PlusOS Dynamic Allocator & Slabs ---\n");
    printf("Allocated Payload:  %lu bytes in %lu active chunks\n",
           (unsigned long)st.heap_alloc_bytes, (unsigned long)st.heap_chunks_active);
    printf("Slab Caches Used:   %lu bytes (%lu free pool slots)\n",
           (unsigned long)st.slab_alloc_bytes, (unsigned long)st.slab_free_slots);
    printf("GC Total Reclaimed: %lu bytes\n", (unsigned long)st.gc_reclaimed_total);
    printf("Heap Integrity:     %s\n",
           st.corruption_detected == 0 ? "OK (All magic headers & tail canaries intact)"
                                       : "WARNING: Corrupted block(s) detected!");

    /* Show per-process memory consumption */
    struct plusos_procinfo p[16];
    struct plusos_proclist_args pl = { p, 16 };
    int n = (int)plusos_syscall(SYS_proclist, &pl);

    printf("\n--- Per-Process RAM Usage ---\n");
    printf("PID  TYPE   STATE    RAM (STACK+IMG)  NAME\n");
    for (int i = 0; i < n; i++) {
        const char *state_str = "-";
        switch (p[i].state) {
        case 1: state_str = "ready"; break;
        case 2: state_str = "run"; break;
        case 3: state_str = "block"; break;
        case 4: state_str = "zombie"; break;
        default: break;
        }
        printf("%3d  %-4s   %-7s  %4lu KB          %s\n",
               p[i].pid,
               p[i].ring == 0 ? "sys" : "user",
               state_str,
               (unsigned long)(p[i].mem_bytes >> 10),
               p[i].name);
    }

    return 0;
}
