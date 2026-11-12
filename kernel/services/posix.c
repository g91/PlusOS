/* posix.c - PlusOS POSIX Process, Identity, Time, and Terminal Support */
#include "kernel.h"
#include "proc.h"
#include "vfs.h"
#include "posix.h"
#include "../proc/signal.h"
#include "environ.h"

static char g_hostname[32] = "plusos";
static struct plusos_termios g_termios = {
    0x0001, /* ICRNL */
    0x0001, /* OPOST */
    0x00BF, /* CS8 | CREAD */
    0x0007, /* ECHO | ICANON | ISIG */
    42, 64,
    9600
};

void posix_init(void)
{
    strlcpy(g_hostname, "plusos", sizeof g_hostname);
    env_set("HOSTNAME", g_hostname);
    g_termios.rows = (uint16_t)con_rows();
    g_termios.cols = (uint16_t)con_cols();
}

long posix_syscall(struct plusos_posix_args *a)
{
    if (!a) return -22;
    switch (a->op) {
    case PLUSOS_POSIX_PIPE:
        return vfs_pipe_alloc(&a->a, &a->b);

    case PLUSOS_POSIX_SPAWN_REDIR:
        return exec_spawn_fds(a->name, a->argc, a->argv, a->fds);

    case PLUSOS_POSIX_ISATTY:
        return vfs_fd_is_console(a->a) ? 1 : 0;

    case PLUSOS_POSIX_GETHOSTNAME:
        if (!a->ptr || a->size <= 0) return -22;
        strlcpy((char *)a->ptr, g_hostname, (size_t)a->size);
        return 0;

    case PLUSOS_POSIX_SETHOSTNAME:
        if (proc_current && proc_current->uid != 0) return -1;
        if (!a->name || !a->name[0]) return -22;
        strlcpy(g_hostname, a->name, sizeof g_hostname);
        env_set("HOSTNAME", g_hostname);
        return 0;

    case PLUSOS_POSIX_UNAME: {
        struct plusos_utsname *u = (struct plusos_utsname *)a->ptr;
        if (!u) return -22;
        memset(u, 0, sizeof *u);
        strlcpy(u->sysname, "PlusOS", sizeof u->sysname);
        strlcpy(u->nodename, g_hostname, sizeof u->nodename);
        strlcpy(u->release, PLUSOS_VERSION, sizeof u->release);
        snprintf(u->version, sizeof u->version, "%s (%s)", hw_machine_name(), hw_cpu_name());
        strlcpy(u->machine, hw_cpu_name(), sizeof u->machine);
        return 0;
    }

    case PLUSOS_POSIX_UMASK: {
        uint16_t old = proc_current ? proc_current->umask : 0022;
        if (proc_current) proc_current->umask = (uint16_t)(a->a & 0777);
        return (long)old;
    }

    case PLUSOS_POSIX_GETUMASK:
        return (long)(proc_current ? proc_current->umask : 0022);

    case PLUSOS_POSIX_TIME: {
        ULONGINT mac_secs = 0;
        ReadDateTime(&mac_secs);
        return (mac_secs >= MAC_UNIX_EPOCH_DELTA) ? (long)(mac_secs - MAC_UNIX_EPOCH_DELTA) : 0;
    }

    case PLUSOS_POSIX_SETTIME: {
        if (proc_current && proc_current->uid != 0) return -1;
        ULONGINT mac_secs = a->uval + MAC_UNIX_EPOCH_DELTA;
        return SetDateTime(mac_secs) == 0 ? 0 : -1;
    }

    case PLUSOS_POSIX_CHROOT: {
        if (proc_current && proc_current->uid != 0) return -1;
        char norm[128];
        vfs_normalize_path(a->name, norm, sizeof norm, 1);
        struct plusos_stat st;
        struct plusos_vfs_args va;
        memset(&va, 0, sizeof va);
        va.op = PLUSOS_VFS_STAT;
        va.path = norm;
        va.buf = &st;
        if (vfs_syscall(&va) != 0) return -2;
        if ((st.st_mode & 0170000) != 0040000) return -20;
        if (proc_current) {
            strlcpy(proc_current->root_dir, norm, sizeof proc_current->root_dir);
            strlcpy(proc_current->cwd, "/", sizeof proc_current->cwd);
        }
        return 0;
    }

    case PLUSOS_POSIX_STTY_GET: {
        struct plusos_termios *t = (struct plusos_termios *)a->ptr;
        if (!t) return -22;
        g_termios.rows = (uint16_t)con_rows();
        g_termios.cols = (uint16_t)con_cols();
        *t = g_termios;
        return 0;
    }

    case PLUSOS_POSIX_STTY_SET: {
        const struct plusos_termios *t = (const struct plusos_termios *)a->ptr;
        if (!t) return -22;
        g_termios = *t;
        return 0;
    }

    case PLUSOS_POSIX_HOSTID: {
        uint32_t id = 0x504C0000UL ^ (uint32_t)(g_hw.rom_version);
        for (int i = 0; g_hostname[i]; i++) id = (id * 33u) ^ (uint8_t)g_hostname[i];
        return (long)id;
    }

    case PLUSOS_POSIX_NPROC:
        return 1;

    case PLUSOS_POSIX_KILL_SIG:
        return signal_send(a->a, a->b);

    case PLUSOS_POSIX_GETPPID:
        return proc_current ? proc_current->parent : 0;

    case PLUSOS_POSIX_GETUID:
        return proc_current ? (long)proc_current->uid : 0;

    case PLUSOS_POSIX_GETGID:
        return proc_current ? (long)proc_current->gid : 0;

    case PLUSOS_POSIX_GETENV_ALL:
        if (!a->ptr || a->size <= 0) return -22;
        return env_list((char *)a->ptr, a->size);

    case PLUSOS_POSIX_CLEARENV: {
        char buf[512];
        int n = env_list(buf, sizeof buf);
        int i = 0;
        while (i < n && buf[i]) {
            char key[ENV_KEY_LEN];
            int k = 0;
            while (i < n && buf[i] && buf[i] != '=' && buf[i] != '\n' && k < ENV_KEY_LEN - 1)
                key[k++] = buf[i++];
            key[k] = 0;
            while (i < n && buf[i] && buf[i] != '\n') i++;
            if (buf[i] == '\n') i++;
            if (key[0]) env_unset(key);
        }
        return 0;
    }

    default:
        return -38;
    }
}
