/* test_kmodule.c - host-side unit test for the kernel module registry, driver
 * table and watchdog (kernel/core/kmodule.c). Not part of the OS build.
 *
 *   gcc -DKMOD_HOST -D'KMOD_NOW()=(fake_now++)' -Ikernel/core \
 *       tools/tests/test_kmodule.c kernel/core/kmodule.c -o /tmp/test_kmodule && /tmp/test_kmodule
 * (or: make test-kmodule) */
#include <stdio.h>
#include <string.h>
#include "kmodule.h"

uint32_t fake_now;
static int order[16], norder;
static int fini_order[16], nfini;

static int ok_a(void)   { order[norder++] = 1; return 0; }
static int ok_b(void)   { order[norder++] = 2; return 0; }
static int bad_c(void)  { order[norder++] = 3; return -5; }
static int ok_d(void)   { order[norder++] = 4; return 0; }   /* depends on c (failed) */
static int ok_e(void)   { order[norder++] = 5; return 0; }   /* depends on a and b */
static int ok_f(void)   { order[norder++] = 6; return 0; }   /* later stage */
static void fin_a(void) { fini_order[nfini++] = 1; }
static void fin_b(void) { fini_order[nfini++] = 2; }

static kmodule tab[] = {
    { "a", KMOD_CORE,    KSTAGE_MEMORY,   KMOD_F_ESSENTIAL, ok_a,  fin_a, 0,      0, 0, 0 },
    { "b", KMOD_SERVICE, KSTAGE_MEMORY,   0,                ok_b,  fin_b, "a",    0, 0, 0 },
    { "c", KMOD_DRIVER,  KSTAGE_BASE,     0,                bad_c, 0,     0,      0, 0, 0 },
    { "d", KMOD_SERVICE, KSTAGE_BASE,     0,                ok_d,  0,     "c",    0, 0, 0 },
    { "e", KMOD_SERVICE, KSTAGE_BASE,     0,                ok_e,  0,     "a b",  0, 0, 0 },
    { "f", KMOD_SERVICE, KSTAGE_DRIVERS,  0,                ok_f,  0,     "e",    0, 0, 0 },
};

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static long drv_ctl(int cmd, void *arg) { (void)arg; return cmd * 2; }
static int irq_hits;
static void drv_irq(void) { irq_hits++; }

int main(void)
{
    kmod_register_table(tab, 6);
    CHECK(kmod_count() == 6);
    CHECK(kmod_find("b") == &tab[1]);
    CHECK(kmod_find("zzz") == 0);

    CHECK(kmod_boot_stage(KSTAGE_MEMORY) == 0);
    CHECK(kmod_running("a") && kmod_running("b"));
    CHECK(kmod_boot_stage(KSTAGE_BASE) == 2);           /* c failed, d skipped */
    CHECK(tab[2].state == KMOD_FAILED && tab[2].err == -5);
    CHECK(tab[3].state == KMOD_SKIPPED);
    CHECK(kmod_running("e"));
    CHECK(kmod_boot_stage(KSTAGE_DRIVERS) == 0 && kmod_running("f"));
    CHECK(norder == 5 && order[0] == 1 && order[1] == 2 && order[2] == 3 && order[3] == 5 && order[4] == 6);
    CHECK(kmod_boot_stage(KSTAGE_BASE) == 0);           /* a second pass starts nothing twice */
    CHECK(norder == 5);
    CHECK(tab[0].init_ticks == 1);                       /* fake clock advances 1 per read pair */
    CHECK(kmod_init_cur == 0);

    kmod_shutdown();                                    /* reverse start order: b's fini before a's */
    CHECK(nfini == 2 && fini_order[0] == 2 && fini_order[1] == 1);
    CHECK(tab[0].state == KMOD_STOPPED);

    /* driver table */
    static const kdriver d1 = { "kbd", KDRV_INPUT, 0, 0, 0, drv_ctl, drv_irq };
    static const kdriver d2 = { "snd", KDRV_SOUND, 0, 0, 0, 0, 0 };
    CHECK(kdrv_register(&d1) == 0 && kdrv_register(&d2) == 0);
    CHECK(kdrv_register(&d1) == -1);                    /* duplicate refused */
    CHECK(kdrv_find("snd") == &d2 && kdrv_find("x") == 0);
    CHECK(kdrv_find_class(KDRV_INPUT) == &d1);
    CHECK(kdrv_control("kbd", 21, 0) == 42);
    CHECK(kdrv_control("snd", 1, 0) == -1);             /* no control() */
    CHECK(kdrv_control("nope", 1, 0) == -1);
    kdrv_irq_all();
    CHECK(irq_hits == 1);
    CHECK(kdrv_count() == 2);

    /* watchdog: trips after the limit, reports on the next pet, resets */
    for (int i = 0; i < KWD_LIMIT_TICKS - 1; i++) kwd_tick();
    CHECK(kwd_stalls() == 0);
    CHECK(kwd_pet() == 0);                              /* healthy: nothing to report */
    for (int i = 0; i < KWD_LIMIT_TICKS + 30; i++) kwd_tick();
    CHECK(kwd_stalls() == 1);
    CHECK(kwd_pet() == KWD_LIMIT_TICKS + 30);
    CHECK(kwd_worst() == KWD_LIMIT_TICKS + 30);
    CHECK(kwd_pet() == 0);

    printf(fails ? "%d FAILED\n" : "all kmodule tests passed\n", fails);
    return fails != 0;
}
