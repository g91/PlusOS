/* reg - view and change the system registry (the key=value settings store
 * kept in "PlusOS Registry" on the boot disk).
 *
 *   reg                 list every entry
 *   reg KEY             show one value
 *   reg KEY VALUE...    set a value
 *   reg -d KEY          delete a key
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(int argc, char **argv)
{
    struct plusos_reg_args a;
    char k[40], v[72];
    memset(&a, 0, sizeof a);
    if (argc < 2) {
        int n = 0;
        for (a.index = 0;; a.index++) {
            a.kbuf = k; a.ksize = sizeof k; a.vbuf = v; a.vsize = sizeof v;
            if (plusos_syscall(SYS_regenum, &a) != 0) break;
            printf("%-30s %s\n", k, v);
            n++;
        }
        if (!n) printf("(the registry is empty)\n");
        return 0;
    }
    if (!strcmp(argv[1], "-d")) {
        if (argc < 3) { printf("usage: reg -d KEY\n"); return 1; }
        a.key = argv[2]; a.value = "";
        return plusos_syscall(SYS_regset, &a) == 0 ? 0 : 1;
    }
    a.key = argv[1];
    if (argc == 2) {
        a.vbuf = v; a.vsize = sizeof v;
        if (plusos_syscall(SYS_regget, &a) < 0) { printf("%s: not set\n", argv[1]); return 1; }
        printf("%s\n", v);
        return 0;
    }
    char val[72]; int n = 0;
    for (int i = 2; i < argc; i++) {
        for (const char *s = argv[i]; *s && n < (int)sizeof val - 2; s++) val[n++] = *s;
        if (i + 1 < argc && n < (int)sizeof val - 2) val[n++] = ' ';
    }
    val[n] = 0;
    a.value = val;
    if (plusos_syscall(SYS_regset, &a) != 0) { printf("could not save\n"); return 1; }
    return 0;
}
