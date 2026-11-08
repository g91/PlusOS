/* security.h - PlusOS Syscall Security Sentinel & Pointer Guard */
#ifndef PLUSOS_SECURITY_H
#define PLUSOS_SECURITY_H

#include <stdint.h>

void security_init(void);
int  sec_validate_ptr(const void *ptr, uint32_t len, int writeable);

#endif
