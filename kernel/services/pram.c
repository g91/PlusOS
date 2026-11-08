/* pram.c - PlusOS Parameter RAM (PRAM) & Real-Time Clock Manager */
#include "kernel.h"
#include "pram.h"

void pram_init(void)
{
}

int pram_read_byte(int offset, uint8_t *val)
{
    if (offset < 0 || offset >= 256 || !val) return -1;
    if (offset < 20) {
        /* Low-memory shadow of the 20-byte base PRAM (SysParam) */
        *val = LM_BYTE(0x1F8 + offset);
        return 0;
    }
    *val = 0;
    return 0;
}

int pram_write_byte(int offset, uint8_t val)
{
    if (offset < 0 || offset >= 256) return -1;
    if (offset < 20) {
        LM_BYTE(0x1F8 + offset) = val;
        WriteParam();
        return 0;
    }
    return 0;
}

int pram_dispatch(struct plusos_pram_args *args)
{
    if (!args) return -1;
    if (args->op == PLUSOS_PRAM_READ) {
        return pram_read_byte(args->offset, &args->value);
    } else if (args->op == PLUSOS_PRAM_WRITE) {
        return pram_write_byte(args->offset, args->value);
    }
    return -1;
}
