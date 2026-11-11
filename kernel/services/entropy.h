/* entropy.h - PlusOS Hardware Entropy Pool & Random Number Generator */
#ifndef PLUSOS_ENTROPY_H
#define PLUSOS_ENTROPY_H

#include <stdint.h>
#include <plusos/syscall.h>
#include "kconfig.h"

#if CONFIG_ENTROPY
void entropy_init(void);
void entropy_harvest(uint32_t val);
int  entropy_get_bytes(void *buf, int len);
#else
static inline void entropy_init(void) {}
static inline void entropy_harvest(uint32_t v) { (void)v; }
static inline int  entropy_get_bytes(void *b, int n) { (void)b; (void)n; return -1; }
#endif

#endif