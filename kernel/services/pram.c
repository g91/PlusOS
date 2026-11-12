/* pram.c - PlusOS Parameter RAM (20B PRAM + 256B XPRAM) Manager */
#include "kernel.h"
#include "pram.h"

void pram_init(void)
{
}

static void xpram_io(uint16_t trap_word, int offset, uint8_t *buf)
{
    uint32_t ctl = (1UL << 16) | (uint32_t)(offset & 0xFF);
    register uint32_t d0 __asm__("d0") = ctl;
    register uint8_t *a0 __asm__("a0") = buf;
    if (trap_word == 0xA051)
        __asm__ volatile(".short 0xA051" : "+d"(d0), "+a"(a0) : : "d1", "d2", "a1", "cc", "memory");
    else
        __asm__ volatile(".short 0xA052" : "+d"(d0), "+a"(a0) : : "d1", "d2", "a1", "cc", "memory");
}

int pram_read_byte(int offset, uint8_t *val)
{
    if (offset < 0 || offset >= 256 || !val) return -1;
    if (offset < 20) {
        *val = LM_BYTE(0x1F8 + offset);
        return 0;
    }
    if (g_hw.has_xpram) {
        xpram_io(0xA051, offset, val);
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
    if (g_hw.has_xpram) {
        xpram_io(0xA052, offset, &val);
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
