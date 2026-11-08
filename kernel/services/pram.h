/* pram.h - PlusOS Parameter RAM (PRAM) & Real-Time Clock Manager */
#ifndef PLUSOS_PRAM_H
#define PLUSOS_PRAM_H

#include <stdint.h>
#include <plusos/syscall.h>

void pram_init(void);
int  pram_read_byte(int offset, uint8_t *val);
int  pram_write_byte(int offset, uint8_t val);
int  pram_dispatch(struct plusos_pram_args *args);

#endif
