/* login.c - interactive first-run wizard and login prompt.
 *
 * These run at the kernel console before init_task hands off to the shell or
 * desktop. The console is already up (con_init in kernel_start) and the
 * keyboard driver is registered by its module. For Phase 1 everything stays
 * text-mode: the window server and GUI apps have not started yet.
 */
#include "kernel.h"
#include "kconfig.h"
#include "users.h"
#include "login.h"
#include "proc.h"
#include "environ.h"
#include "perms.h"

#if CONFIG_USERS

/* Read a line from the kernel console. echo=0 draws '*' per character (for
 * password entry). Enter/Return ends the line. Backspace erases. Non-printing
 * keys are ignored. */
static int read_line(char *buf, int cap, int echo)
{
    int n = 0;
    for (;;) {
        int k = kbd_get();
        if (k == 0x0D || k == 0x0A) { printf("\n"); buf[n] = 0; return n; }
        if (k == 0x08 || k == 0x7F) {         /* backspace / delete */
            if (n > 0) { n--; printf("\b \b"); }
            continue;
        }
        if (k < 0x20 || k > 0x7E) continue;
        if (n >= cap - 1) continue;
        buf[n++] = (char)k;
        printf("%c", echo ? k : '*');
    }
}

static int yes_no(const char *prompt, int default_yes)
{
    printf("%s [%s]: ", prompt, default_yes ? "Y/n" : "y/N");
    char ans[8];
    int n = read_line(ans, sizeof ans, 1);
    if (n == 0) return default_yes;
    char c = ans[0];
    if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
    return (c == 'y') ? 1 : (c == 'n') ? 0 : default_yes;
}

/* Lowercase, strip spaces, 1-15 chars, [a-z0-9_-]. Returns 0 on success. */
static int normalise_name(char *name)
{
    int w = 0;
    for (int r = 0; name[r]; r++) {
        char c = name[r];
        if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_') {
            if (w < USER_NAME_MAX - 1) name[w++] = c;
        }
    }
    name[w] = 0;
    return w > 0 ? 0 : -1;
}

void login_firstrun(void)
{
    if (!users_firstrun_needed()) return;

    printf("\n");
    printf("==============================================\n");
    printf("  Welcome to PlusOS - first-run setup\n");
    printf("==============================================\n");
    printf("No user accounts were found on this disk.\n");
    printf("Let's create a root account and your own.\n\n");

    /* Root first, with the default password "Apple" the user asked for.
     * Prompt to change it; whatever they type (or empty = keep default) wins. */
    printf("The root (administrator) account will be created with the\n");
    printf("default password \"Apple\". You can change it now or later with\n");
    printf("the passwd command.\n\n");
    char root_pw[32];
    printf("New root password (press Return to keep \"Apple\"): ");
    read_line(root_pw, sizeof root_pw, 0);
    if (root_pw[0] == 0) {
        strncpy(root_pw, "Apple", sizeof root_pw - 1);
        root_pw[sizeof root_pw - 1] = 0;
    }
    if (users_add("root", root_pw, 1, 1) != 0)
        printf("warning: could not create the root account.\n");
    else
        printf("Root account created.\n\n");

    /* Primary user. */
    char name[USER_NAME_MAX];
    for (;;) {
        printf("Pick a username (letters, digits, '-' or '_'): ");
        read_line(name, sizeof name, 1);
        if (normalise_name(name) == 0 && strcmp(name, "root") != 0) break;
        printf("  please enter a name of at least one letter, not 'root'.\n");
    }

    char pw1[32], pw2[32];
    int want_pw = yes_no("Do you want a password on this account?", 1);
    if (want_pw) {
        for (;;) {
            printf("Password: ");
            read_line(pw1, sizeof pw1, 0);
            printf("Confirm:  ");
            read_line(pw2, sizeof pw2, 0);
            if (strcmp(pw1, pw2) == 0 && pw1[0]) break;
            printf("  passwords did not match (or were empty); try again.\n");
        }
    } else {
        pw1[0] = 0;
    }

    int sudoer = yes_no("Grant this account root (sudo) permissions?", 1);
    if (users_add(name, pw1, sudoer, 0) != 0) {
        printf("error: could not create the user account.\n");
        return;
    }

    /* Seed sensible default permissions. /etc/shadow must be root-only; the
     * user owns their home and its subdirectories. This is the only place
     * that calls perms_set directly at uid 0 without a chmod/chown check. */
    user_rec u;
    if (users_find(name, &u) == 0) {
        perms_set("/etc",           0755, 0, 0);
        perms_set("/etc/passwd",    0644, 0, 0);
        perms_set("/etc/shadow",    0600, 0, 0);
        perms_set("/etc/perms.db",  0600, 0, 0);
        perms_set("/root",          0700, 0, 0);
        perms_set("/home",          0755, 0, 0);
        char p[USER_HOME_MAX + 20];
        snprintf(p, sizeof p, "/home/%s", u.name);              perms_set(p, 0755, u.uid, u.gid);
        snprintf(p, sizeof p, "/home/%s/Desktop",   u.name);    perms_set(p, 0755, u.uid, u.gid);
        snprintf(p, sizeof p, "/home/%s/Documents", u.name);    perms_set(p, 0755, u.uid, u.gid);
        snprintf(p, sizeof p, "/home/%s/Settings",  u.name);    perms_set(p, 0700, u.uid, u.gid);
    }

    printf("\n");
    printf("Account '%s' created (uid %d)%s.\n", name, 1000,
           sudoer ? ", with sudo rights" : "");
    printf("Home directory: /home/%s\n", name);
    printf("You can now log in.\n\n");
}

void login_prompt(void)
{
    if (users_count() == 0) return;              /* nothing to log in against */
    char name[USER_NAME_MAX], pw[32];
    for (;;) {
        printf("\nPlusOS login: ");
        if (read_line(name, sizeof name, 1) <= 0) continue;
        normalise_name(name);
        printf("Password: ");
        read_line(pw, sizeof pw, 0);
        if (users_authenticate(name, pw)) {
            user_rec u;
            if (users_find(name, &u) == 0) {
                users_set_current(u.uid);
                /* Stamp the running task so children (shell, desktop, ...)
                 * inherit this identity via proc_create_*. */
                if (proc_current) {
                    proc_current->uid = u.uid;
                    proc_current->gid = u.gid;
                }
                env_set("HOME", u.home);
                env_set("USER", u.name);
                env_set("SHELL", u.shell);
                printf("\nWelcome, %s. Home: %s\n\n", u.name, u.home);
            }
            return;
        }
        printf("Login incorrect.\n");
    }
}

#endif /* CONFIG_USERS */
