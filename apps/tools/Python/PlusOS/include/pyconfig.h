/* pyconfig.h - hand-written CPython 3.15 configuration for PlusOS (Motorola 68000, big-endian,
 * single-threaded, no MMU, no dynamic loading, no FPU: floats go through p68 softfloat). */
#ifndef Py_PYCONFIG_H
#define Py_PYCONFIG_H

#define PLUSOS 1
#define static_assert _Static_assert
#define _PYTHONFRAMEWORK ""
#define F_GETFD 1
#define F_SETFD 2
#define FD_CLOEXEC 1
#define F_GETFL 3
#define F_SETFL 4
#define __thread
#define Py_NO_ENABLE_SHARED 1
#define HAVE_PTHREAD_STUBS 1
#define WORDS_BIGENDIAN 1
#define DOUBLE_IS_BIG_ENDIAN_IEEE754 1
#define HAVE_DOUBLE_IS_BIG_ENDIAN_IEEE754 1

/* sizes (int == long == pointer == 32 bits on this target) */
#define SIZEOF_CHAR 1
#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8
#define SIZEOF_VOID_P 4
#define SIZEOF_SIZE_T 4
#define SIZEOF_WCHAR_T 4
#define SIZEOF_OFF_T 4
#define SIZEOF_TIME_T 4
#define SIZEOF_PID_T 4
#define SIZEOF_PTHREAD_T 4
#define ALIGNOF_LONG 2
#define ALIGNOF_INT 2
#define ALIGNOF_LONG_LONG 2
#define ALIGNOF_DOUBLE 2
#define ALIGNOF_VOID_P 2
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_UTSNAME_H 1
#define PLATLIBDIR "lib"
#define HAVE_LANGINFO_H 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_NL_LANGINFO 1
#define HAVE_SIGACTION 0
#undef HAVE_SIGACTION
#define SIZEOF_UINTPTR_T 4
#define SIZEOF_FPOS_T 4
#define SIZEOF__BOOL 1
#define ALIGNOF_MAX_ALIGN_T 2
#define ALIGNOF_SIZE_T 2
#define HAVE_LONG_LONG 1
#define HAVE_UINTPTR_T 1
#define HAVE_INTPTR_T 1
#define HAVE_SSIZE_T 1
#define HAVE_WCHAR_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_FCNTL_H 1
#define HAVE_DIRENT_H 1
#define HAVE_LIMITS_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_ERRNO_H 1
#define HAVE_TERMIOS_H 1
#define HAVE_UTIME_H 1
#define HAVE_LOCALE_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STRING_H 1
#define HAVE_MEMORY_H 1
#define STDC_HEADERS 1
#define HAVE_STDDEF_H 1
#define HAVE_GETCWD 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRFTIME 1
#define HAVE_MKTIME 1
#define HAVE_FSTAT 1
#define HAVE_STAT 1
#define HAVE_LSTAT 1
#define HAVE_FTRUNCATE 1
#define HAVE_CHMOD 1
#define HAVE_GETPID 1
#define HAVE_GETPPID 1
#define HAVE_GETUID 1
#define HAVE_GETEUID 1
#define HAVE_GETGID 1
#define HAVE_GETEGID 1
#define HAVE_DUP 1
#define HAVE_DUP2 1
#define HAVE_PIPE 1
#define HAVE_READLINK 1
#define HAVE_REALPATH 1
#define HAVE_SYMLINK 1
#define HAVE_LINK 1
#define HAVE_UMASK 1
#define HAVE_UTIME 1
#define HAVE_KILL 1
#define HAVE_ALARM 1
#define HAVE_NICE 1
#define HAVE_UNAME 1
#define HAVE_SETENV 1
#define HAVE_UNSETENV 1
#define HAVE_MKFIFO 1
#define HAVE_MKNOD 1
#define HAVE_SYNC 1
#define HAVE_TRUNCATE 1
#define HAVE_ISATTY 1
#define HAVE_NANOSLEEP 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_STRNDUP 1
#define HAVE_MKDTEMP 1
#define HAVE_MKSTEMP 1
#define HAVE_SETLOCALE 1
#define HAVE_LOCALECONV 1
#define HAVE_CLOCK 1
#define HAVE_SYS_TIMES_H 1
#define HAVE_TIMES 1
#define HAVE_TZNAME 1
#define HAVE_TM_ZONE 0
#undef HAVE_TM_ZONE
#define HAVE_GETHOSTNAME 1
#define HAVE_LCHOWN 0
#undef HAVE_LCHOWN
#define HAVE_CHOWN 1
#define HAVE_FCHDIR 0
#undef HAVE_FCHDIR
#define HAVE_FSYNC 0
#undef HAVE_FSYNC
#define HAVE_FLOCK 0
#undef HAVE_FLOCK
#define HAVE_GETENTROPY 0
#undef HAVE_GETENTROPY

/* math */
#define HAVE_COPYSIGN 1
#define HAVE_ROUND 1
#define HAVE_HYPOT 1
#define HAVE_ATAN2 1
#define HAVE_ACOSH 1
#define HAVE_ASINH 1
#define HAVE_ATANH 1
#define HAVE_EXPM1 1
#define HAVE_LOG1P 1
#define HAVE_LOG2 1
#define HAVE_ERF 1
#define HAVE_ERFC 1
#define HAVE_TGAMMA 1
#define HAVE_LGAMMA 1
#define HAVE_FMOD 1
#define HAVE_DECL_ISNAN 1
#define HAVE_DECL_ISINF 1
#define HAVE_DECL_ISFINITE 1
#define HAVE_DECL_ISNAN 1
#define _PY_SHORT_FLOAT_REPR 1
#define PY_NO_SHORT_FLOAT_REPR 0
#undef PY_NO_SHORT_FLOAT_REPR
#define X87_DOUBLE_ROUNDING 0
#undef X87_DOUBLE_ROUNDING

/* what we deliberately do NOT have */
#undef WITH_PYMALLOC_DISABLED
#define WITH_PYMALLOC 1
#undef Py_GIL_DISABLED
#undef Py_DEBUG
#undef HAVE_DLOPEN
#undef HAVE_PTHREAD_H
#undef WITH_THREAD
#undef Py_TRACE_REFS
#define PY_LONG_BITS_IN_DIGIT 0
#undef PY_LONG_BITS_IN_DIGIT

#define Py_ENABLE_SHARED 0
#undef Py_ENABLE_SHARED
#define PLATFORM "plusos"
#define MULTIARCH "m68k-plusos"
#define PY_SUPPORT_TIER 3
#define Py_HAVE_C_COMPLEX 1
#undef Py_HAVE_C_COMPLEX
#define Py_STATS 0
#undef Py_STATS
#define PY_FORMAT_SIZE_T "z"
#define ABIFLAGS ""
#define VERSION "3.15"
#define VPATH "."
#define PREFIX "/python"
#define EXEC_PREFIX "/python"
#define PYTHONPATH "/python/lib"
#define SOABI "plusos-m68k"
#define _Py_FASTCALL
#define _GNU_SOURCE 1
#define _POSIX_C_SOURCE 200809L

/* Single-threaded: map the GCC __atomic builtins CPython's pyatomic_gcc.h uses onto plain
 * memory operations (the p68 compiler has neither <stdatomic.h> nor __atomic_*). */
#define _Py_USE_GCC_BUILTIN_ATOMICS 1
#define __ATOMIC_RELAXED 0
#define __ATOMIC_CONSUME 1
#define __ATOMIC_ACQUIRE 2
#define __ATOMIC_RELEASE 3
#define __ATOMIC_ACQ_REL 4
#define __ATOMIC_SEQ_CST 5
#define __atomic_load_n(p,o) (*(p))
#define __atomic_store_n(p,v,o) ((void)(*(p)=(v)))
#define __atomic_fetch_add(p,v,o) ({ typeof(*(p)) _ao=*(p); *(p)=_ao+(v); _ao; })
#define __atomic_fetch_sub(p,v,o) ({ typeof(*(p)) _ao=*(p); *(p)=_ao-(v); _ao; })
#define __atomic_fetch_and(p,v,o) ({ typeof(*(p)) _ao=*(p); *(p)=_ao&(v); _ao; })
#define __atomic_fetch_or(p,v,o) ({ typeof(*(p)) _ao=*(p); *(p)=_ao|(v); _ao; })
#define __atomic_exchange_n(p,v,o) ({ typeof(*(p)) _ao=*(p); *(p)=(v); _ao; })
#define __atomic_compare_exchange_n(p,e,d,w,s,f) ({ int _ar; if (*(p)==*(e)) { *(p)=(d); _ar=1; } else { *(e)=*(p); _ar=0; } _ar; })
#define __atomic_thread_fence(o) ((void)0)
#define __atomic_signal_fence(o) ((void)0)

#ifndef __ASSEMBLER__
typedef int clockid_t;
typedef int sig_atomic_t;
#ifndef SSIZE_MAX
#define SSIZE_MAX 0x7fffffffL
#endif
#ifndef BUFSIZ
#define BUFSIZ 1024
#endif
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#endif

#ifndef __ASSEMBLER__
#include <errno.h>
#include <fcntl.h>
#include <time.h>
/* errno values the PlusOS libc header does not define (Linux numbering) */
#define PY_E(n, v) 
#ifndef EALREADY
#define EALREADY 114
#define EINPROGRESS 115
#define ENOTSOCK 88
#define EDESTADDRREQ 89
#define EMSGSIZE 90
#define EPROTOTYPE 91
#define ENOPROTOOPT 92
#define EPROTONOSUPPORT 93
#define EOPNOTSUPP 95
#define ENOTSUP 95
#define EAFNOSUPPORT 97
#define EADDRINUSE 98
#define EADDRNOTAVAIL 99
#define ENETDOWN 100
#define ENETUNREACH 101
#define ECONNABORTED 103
#define ECONNRESET 104
#define ENOBUFS 105
#define EISCONN 106
#define ENOTCONN 107
#define ETIMEDOUT 110
#define ECONNREFUSED 111
#define EHOSTUNREACH 113
#define EWOULDBLOCK EAGAIN
#define EDEADLK 35
#define ENOLCK 37
#define EOVERFLOW 75
#define EILSEQ 84
#define ENOTBLK 15
#define ETXTBSY 26
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#endif

#ifndef __ASSEMBLER__
#include <signal.h>
#include <sys/utsname.h>
#ifndef SIGCHLD
#define SIGCHLD 17
#define SIGBUS 7
#define SIGFPE 8
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGWINCH 28
#define SIGTRAP 5
#define SIGXFSZ 25
#define SIGXCPU 24
#define SIGVTALRM 26
#define SIGPROF 27
#define SIGIO 29
#define SIGSYS 31
#define NSIG 32
#endif
typedef long clock_t;
#define CLOCKS_PER_SEC 1000000L
clock_t clock(void);
extern long timezone;
extern int daylight;
extern char *tzname[2];
void tzset(void);
#endif
#endif
