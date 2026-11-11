/* pram.h - PlusOS Parameter RAM (PRAM) & Real-Time Clock Manager */
#ifndef PLUSOS_PRAM_H
#define PLUSOS_PRAM_H

#include <stdint.h>
#include <plusos/syscall.h>
#include "kconfig.h"

#if CONFIG_PRAM
void pram_init(void);
int  pram_read_byte(int offset, uint8_t *val);
int  pram_write_byte(int offset, uint8_t val);
int  pram_dispatch(struct plusos_pram_args *args);
#else
static inline void pram_init(void) {}
static inline int  pram_read_byte(int o, uint8_t *v) { (void)o; (void)v; return -1; }
static inline int  pram_write_byte(int o, uint8_t v) { (void)o; (void)v; return -1; }
static inline int  pram_dispatch(struct plusos_pram_args *a) { (void)a; return -1; }
#endif

#endif