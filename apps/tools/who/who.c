/* who - show who is logged on. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    int count_only = 0, boot_t = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-q")) count_only = 1; else if (!strcmp(argv[i], "-b")) boot_t = 1;
    }
    struct plusos_users_args u = { 0 }; u.op = PLUSOS_USERS_WHOAMI;
    int uid = (int)plusos_syscall(SYS_users, &u);
    memset(&u, 0, sizeof u); u.op = PLUSOS_USERS_FIND_UID; u.uid = uid;
    const char *name = (plusos_syscall(SYS_users, &u) == 0 && u.name[0]) ? u.name : "root";
    if (count_only) { printf("%s\n# users=1\n", name); return 0; }
    time_t t = time(NULL);
    if (boot_t) { struct plusos_sysinfo si = { 0 }; plusos_syscall(SYS_sysinfo, &si); t -= (time_t)(si.ticks / 60); }
    struct tm *tm = localtime(&t); char tb[24] = "1970-01-01 00:00";
    if (tm) strftime(tb, sizeof tb, "%Y-%m-%d %H:%M", tm);
    if (boot_t) printf("         system boot  %s\n", tb);
    else printf("%-8s tty0         %s\n", name, tb);
    return 0;
}
