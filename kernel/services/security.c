/* security.c - PlusOS Syscall Security Sentinel & Pointer Guard */
#include "kernel.h"
#include "security.h"

void security_init(void)
{
}

int sec_validate_ptr(const void *ptr, uint32_t len, int writeable)
{
    if (!ptr && len > 0) return -1;
    uint32_t addr = (uint32_t)ptr;
    uint32_t end = addr + len;
    if (end < addr) return -1; /* Overflow */

    /* Protect CPU zero-page vector table 0x0000 - 0x0400 */
    if (writeable && addr < 0x400) return -1;

    /* Protect kernel load area if writeable */
    uint32_t kstart = __plusos_load_addr - KERNEL_STACK;
    uint32_t kend = __plusos_load_addr + 0x30000;
    if (writeable && ((addr >= kstart && addr < kend) || (end > kstart && end <= kend))) {
        return -1;
    }

    /* Range must be within RAM */
    if (end > ram_bytes()) return -1;

    return 0;
}
