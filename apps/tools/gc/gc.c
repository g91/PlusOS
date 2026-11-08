/* gc - PlusOS Kernel Garbage Collector & RAM Compactor CLI */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(int argc, char **argv)
{
    int do_compact = 1;
    if (argc >= 2 && strcmp(argv[1], "--no-compact") == 0) {
        do_compact = 0;
    }

    uint32_t free_before = plusos->mem_free_bytes();
    printf("PlusOS Garbage Collector initiating sweep...\n");
    if (do_compact) {
        printf("Compacting ROM relocatable handles & purging purgeable resources...\n");
    }

    struct plusos_gc_args a;
    memset(&a, 0, sizeof a);
    a.compact_heap = do_compact;

    plusos_syscall(SYS_gc, &a);

    uint32_t free_after = plusos->mem_free_bytes();

    printf("\n--- GC Scavenger Results ---\n");
    printf("Orphaned memory chunks freed: %d\n", a.chunks_freed);
    printf("Zombie processes reaped:     %d\n", a.zombies_reaped);
    printf("Defunct IPC pipes collected: %d\n", a.pipes_closed);
    printf("Dynamic heap memory freed:   %lu bytes\n", (unsigned long)a.bytes_reclaimed);
    printf("ROM heap free before:        %lu KB\n", (unsigned long)(free_before >> 10));
    printf("ROM heap free after:         %lu KB\n", (unsigned long)(free_after >> 10));

    return 0;
}
