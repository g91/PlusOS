/* signal.h - PlusOS Signal Dispatching Subsystem */
#ifndef PLUSOS_SIGNAL_H
#define PLUSOS_SIGNAL_H

#include <stdint.h>
#include <plusos/syscall.h>

void signal_init(void);
int  signal_send(int pid, int signum);
void signal_check(void);

#endif
