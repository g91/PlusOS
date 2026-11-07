/* rt_common.c - start-up support shared by the kernel and apps:
 * global constructors/destructors (C++), atexit, assert. */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef void (*ctor_fn)(void);
extern ctor_fn __init_array_start[], __init_array_end[];
extern ctor_fn __fini_array_start[], __fini_array_end[];

#define MAX_ATEXIT 32
static void (*atexit_fns[MAX_ATEXIT])(void);
static int atexit_count;

void __plusos_run_ctors(void)
{
    for (ctor_fn *p = __init_array_start; p < __init_array_end; p++)
        if (*p && *p != (ctor_fn)-1)
            (*p)();
}

void __plusos_run_dtors(void)
{
    while (atexit_count > 0)
        atexit_fns[--atexit_count]();
    for (ctor_fn *p = __fini_array_end; p > __fini_array_start; ) {
        --p;
        if (*p && *p != (ctor_fn)-1)
            (*p)();
    }
}

int atexit(void (*fn)(void))
{
    if (atexit_count >= MAX_ATEXIT)
        return -1;
    atexit_fns[atexit_count++] = fn;
    return 0;
}

/* C++ ABI: register a destructor for a static object. */
void *__dso_handle = 0;
int __cxa_atexit(void (*fn)(void *), void *arg, void *dso)
{
    (void)dso;
    /* Our atexit list holds plain functions; static-object destructors are
     * rare in PlusOS programs, so we only keep the common no-argument case
     * fully supported and accept (but ignore) the rest. */
    (void)fn; (void)arg;
    return 0;
}

void __plusos_assert_fail(const char *expr, const char *file, int line)
{
    printf("\nASSERT FAILED: %s\n  at %s:%d\n", expr, file, line);
    abort();
}
