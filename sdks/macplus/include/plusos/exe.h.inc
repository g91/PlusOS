/* exe.h.inc - assembler view of <plusos/exe.h> */
#define PEX_FORMAT       1
#define PEX_KIND_KERNEL  1
#define PEX_KIND_APP     2
#ifndef PLUSOS_STACK_SIZE
#ifdef PLUSOS_KERNEL
#define PLUSOS_STACK_SIZE 0x10000      /* kernel stack: 64 KB */
#else
#define PLUSOS_STACK_SIZE 0            /* apps run on the kernel's stack */
#endif
#endif
