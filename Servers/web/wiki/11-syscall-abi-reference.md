# Complete Syscall ABI Reference (SYS 1-63)

PlusOS userland programs enter the kernel via the Motorola 68000 instruction `TRAP #1`.

- **Input Register `D0`**: System call number (`SYS_*`).
- **Input Register `A0`**: Scalar parameter or pointer to argument structure.
- **Output Register `D0`**: Return value (positive/zero on success, negative error code on failure).
- **Registers Preserved**: `D1-D7`, `A1-A6`.

---

## Core System Calls (SYS 1 to SYS 33)

| Syscall | Code | Input (`A0`) | Output (`D0`) | Description |
|---|---|---|---|---|
| `SYS_exit` | 1 | `int exit_code` | — | Terminates calling process. |
| `SYS_yield` | 2 | — | 0 | Relinquishes remainder of current time quantum. |
| `SYS_sleep` | 3 | `uint32_t ticks` | 0 | Puts process to sleep for given 60 Hz ticks. |
| `SYS_getpid` | 4 | — | `int pid` | Returns current process identifier. |
| `SYS_spawn` | 5 | `struct plusos_spawn_args*` | `pid` or `<0` | Spawns a program into a new process. |
| `SYS_wait` | 6 | `int pid` | `exit_code` | Blocks caller until target process exits. |
| `SYS_getkey` | 7 | — | `int key` | Blocking keyboard read. |
| `SYS_pollkey`| 8 | — | `int key` or `0` | Non-blocking keyboard poll. |
| `SYS_readline`| 9| `struct { char* buf; int size; }*` | `length` | Reads line with arrow-key history & tab completion. |
| `SYS_reboot` | 10 | — | — | Cleanly resets Macintosh hardware. |
| `SYS_shutdown`| 11 | — | — | Flushes disk caches and safely halts. |
| `SYS_malloc` | 12 | `uint32_t size` | `void*` | Allocates memory from the ROM heap. |
| `SYS_free` | 13 | `void* ptr` | 0 | Releases memory to ROM heap. |
| `SYS_memfree`| 14 | — | `uint32_t` | Returns free bytes in ROM heap. |
| `SYS_blkio` | 15 | `struct plusos_blkio_args*` | `0` or `OSErr` | Low-level direct sector read/write. |
| `SYS_chainboot`| 16| `int scsi_id` | — | Restarts Mac and boots SCSI drive ID (Mac SE). |
| `SYS_getcfg` | 17 | `int cfg_key` | `value` | Reads configuration value from disk. |
| `SYS_setcfg` | 18 | `struct plusos_cfg_args*` | `0` or `<0` | Writes configuration value to disk. |
| `SYS_sysinfo`| 19 | `struct plusos_sysinfo*` | 0 | Populates machine, RAM, uptime, and disk info. |
| `SYS_kill` | 20 | `int pid` | `0` or `<0` | Sends termination request to process. |
| `SYS_reapany`| 21 | `struct plusos_reap*` | `exit_code` | Reaps any terminated child process without blocking. |
| `SYS_proclist`| 22 | `struct plusos_proclist_args*` | `count` | Returns list of all active processes. |
| `SYS_regget` | 23 | `struct plusos_reg_args*` | `length` | Reads key/value from system registry. |
| `SYS_regset` | 24 | `struct plusos_reg_args*` | 0 | Writes key/value to system registry. |
| `SYS_regenum`| 25 | `struct plusos_reg_args*` | `0` or `-1` | Enumerates all registry keys. |
| `SYS_console`| 26 | `struct plusos_con_args*` | — | Manipulates hardware text console window. |
| `SYS_win` | 27 | `struct plusos_win_args*` | — | Window server interaction (create, post, update). |
| `SYS_termspawn`| 28| `struct plusos_spawn_args*` | `pid` | Spawns process inside dedicated terminal window. |
| `SYS_thread` | 29 | `struct plusos_thread_args*` | `tid` | Spawns lightweight thread sharing process memory. |
| `SYS_detach` | 30 | `int pid` | 0 | Marks process to auto-reap on termination. |
| `SYS_history`| 31 | `struct plusos_history_args*` | — | Persistent 20-command history manager. |
| `SYS_sound` | 32 | `struct plusos_sound_args*` | 0 | 4-voice PWM hardware sound synthesizer. |
| `SYS_scrap` | 33 | `struct plusos_scrap_args*` | — | System-wide clipboard manager. |

---

## Extended Kernel Subsystems (SYS 34 to SYS 63)

| Syscall | Code | Input (`A0`) | Output (`D0`) | Subsystem |
|---|---|---|---|---|
| `SYS_ramstats` | 34 | `struct plusos_ram_stats*` | 0 | Dynamic RAM & Slab Pool Statistics |
| `SYS_gc` | 35 | `struct plusos_gc_args*` | `bytes` | Garbage Collector & Heap Compaction |
| `SYS_heapcheck`| 36 | — | `corruptions` | Canary Sentinel Heap Verifier |
| `SYS_pipecreate`| 37 | `struct plusos_pipe_args*` | `pipe_id` | Circular FIFO Pipe Allocator |
| `SYS_piperead` | 38 | `struct plusos_pipe_io*` | `bytes` | Blocking/Non-blocking Pipe Read |
| `SYS_pipewrite`| 39 | `struct plusos_pipe_io*` | `bytes` | Blocking/Non-blocking Pipe Write |
| `SYS_pipeclose`| 40 | `int pipe_id` | 0 | Closes FIFO Pipe Descriptor |
| `SYS_seminit` | 41 | `struct plusos_sem_args*` | `sem_id` | Counting Semaphore Allocator |
| `SYS_semwait` | 42 | `int sem_id` | 0 | Decrements/Blocks on Semaphore |
| `SYS_sempost` | 43 | `int sem_id` | 0 | Increments/Signals Semaphore |
| `SYS_semdestroy`| 44| `int sem_id` | 0 | Deallocates Semaphore |
| `SYS_mutexinit`| 45 | — | `mutex_id` | Reentrant Mutex Allocator |
| `SYS_mutexlock`| 46 | `int mutex_id` | 0 | Acquires Mutex Lock |
| `SYS_mutexunlock`| 47| `int mutex_id` | 0 | Releases Mutex Lock |
| `SYS_signal` | 48 | `struct plusos_sig_args*` | 0 | Asynchronous Signal Registration & Delivery |
| `SYS_alarm` | 49 | `uint32_t seconds` | `remaining` | Process Real-Time Alarm Timer |
| `SYS_timer` | 50 | `struct plusos_timer_args*` | `timer_id` | Delta Timer Wheel Subsystem |
| `SYS_klog` | 51 | `struct plusos_klog_args*` | `bytes` | Kernel Syslog Ring Buffer Logger |
| `SYS_dmesg` | 52 | `struct plusos_dmesg_args*`| `bytes` | Extracts Kernel Boot Log Messages |
| `SYS_getenv` | 53 | `struct plusos_env_args*` | `length` | Reads Process Environment Variable |
| `SYS_setenv` | 54 | `struct plusos_env_args*` | 0 | Sets/Deletes Process Environment Variable |
| `SYS_ramdisk` | 55 | `struct plusos_ramdisk_args*`| 0 | Virtual High-Speed In-Memory RAM Disk |
| `SYS_shm` | 56 | `struct plusos_shm_args*` | `shmid`/`ptr` | Shared Memory IPC Segments |
| `SYS_power` | 57 | `struct plusos_power_args*`| 0 | Power Management & CRT Screen Saver |
| `SYS_pram` | 58 | `struct plusos_pram_args*` | 0 | Real-Time Clock Parameter RAM (PRAM) |
| `SYS_getrandom`| 59 | `struct plusos_rand_args*` | 0 | Hardware Entropy PRNG Random Generator |
| `SYS_perfmetrics`| 60 | `struct plusos_perf_metrics*`| 0 | System Profiler & Load Averages |
| `SYS_priority` | 61 | `struct plusos_prio_args*` | `old_prio` | Dynamic Process Scheduling Priority |
| `SYS_watchdog` | 62 | `struct plusos_watchdog_args*`| 0 | Hardware/Software Hang Watchdog |
| `SYS_procpgid` | 63 | `struct plusos_pgid_args*` | `pgid` | Process Group & Terminal Arbitration |

---

## Navigation

- **Previous Chapter**: [← 10. Fixed-Point 3D Engine & Graphics](/wiki/10-graphics-and-3d-engine)
- **Next Chapter**: [12. SDK & Application Development Guide →](/wiki/12-sdk-and-app-development)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)
