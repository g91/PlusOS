/* mem - free memory in the ROM heap. */
#include <plusos/plusos.h>

int main(void)
{
    unsigned long f = plusos->mem_free_bytes();
    printf("%lu bytes (%luK) free in the ROM heap\n", f, f >> 10);
    return 0;
}
