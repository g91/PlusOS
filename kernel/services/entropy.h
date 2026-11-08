/* entropy.h - PlusOS Hardware Entropy Pool & Random Number Generator */
#ifndef PLUSOS_ENTROPY_H
#define PLUSOS_ENTROPY_H

#include <stdint.h>
#include <plusos/syscall.h>

void entropy_init(void);
void entropy_harvest(uint32_t val);
int  entropy_get_bytes(void *buf, int len);

#endif
