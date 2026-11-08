/* boot - start another System from a SCSI disk (Mac SE). */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: boot <scsi id>    1 = 6.0.8   2 = 7.1   3 = 7.5.5\n");
        return 1;
    }
    int id = atoi(argv[1]);
    printf("Starting the System on SCSI ID %d...\n", id);
    long rc = plusos_syscall(SYS_chainboot, (void *)(intptr_t)id);   /* restarts on success */
    if (rc == PLUSOS_EBOOT_NOTSE)       printf("Switching Systems needs a Mac SE.\n");
    else if (rc == PLUSOS_EBOOT_NODISK) printf("No SCSI disk at ID %d.\n", id);
    else                                printf("Could not set the startup disk (%ld).\n", rc);
    return 1;
}
