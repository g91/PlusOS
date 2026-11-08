/* entropy.c - PlusOS Hardware Entropy Pool & Random Number Generator */
#include "kernel.h"
#include "entropy.h"

static uint32_t entropy_pool[4];
static uint32_t rand_state[4] = { 0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321 };

void entropy_init(void)
{
    entropy_pool[0] = (uint32_t)TickCount();
    entropy_pool[1] = LM_LONG(0x20C); /* Time */
    entropy_pool[2] = LM_LONG(0x82C); /* Mouse */
    entropy_pool[3] = 0xCAFEBABE;
}

void entropy_harvest(uint32_t val)
{
    entropy_pool[0] ^= val;
    entropy_pool[1] = (entropy_pool[1] << 5) | (entropy_pool[1] >> 27);
    entropy_pool[2] += val * 0x9E3779B9;
    entropy_pool[3] ^= (entropy_pool[0] + entropy_pool[2]);
}

static uint32_t xorshift128(void)
{
    uint32_t t = rand_state[3];
    uint32_t s = rand_state[0];
    rand_state[3] = rand_state[2];
    rand_state[2] = rand_state[1];
    rand_state[1] = s;
    t ^= t << 11;
    t ^= t >> 8;
    rand_state[0] = t ^ s ^ (s >> 19);
    return rand_state[0];
}

int entropy_get_bytes(void *buf, int len)
{
    if (!buf || len <= 0) return -1;
    /* Mix current hardware state into pool */
    entropy_harvest((uint32_t)TickCount() ^ (uint32_t)LM_LONG(0x82C));
    rand_state[0] ^= entropy_pool[0];
    rand_state[1] ^= entropy_pool[1];
    rand_state[2] ^= entropy_pool[2];
    rand_state[3] ^= entropy_pool[3];

    uint8_t *dst = (uint8_t *)buf;
    int filled = 0;
    while (filled < len) {
        uint32_t r = xorshift128();
        int chunk = len - filled;
        if (chunk > 4) chunk = 4;
        memcpy(dst + filled, &r, (size_t)chunk);
        filled += chunk;
    }
    return 0;
}
