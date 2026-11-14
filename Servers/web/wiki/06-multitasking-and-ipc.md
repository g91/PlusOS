# Preemptive Multitasking & IPC Subsystems

PlusOS transforms the vintage Macintosh from a single-tasking computer into a modern multitasking workstation.

---

## 1. Preemptive Scheduler (`kernel/proc/sched.c`)

Unlike Mac OS System 7's cooperative `WaitNextEvent` mechanism, PlusOS utilizes a **true preemptive round-robin scheduler**:
- **Timer Tick Interruption**: Driven by the 60 Hz vertical blanking (VBL) interrupt and 6522 VIA timer.
- **Preemption**: When a user process's time quantum expires, the timer interrupt saves the CPU register state (`D0-D7/A0-A6`, `PC`, `SR`, and `USP`) onto the process stack and schedules the next runnable task.
- **Heartbeat Task**: A dedicated Ring-0 kernel task runs continuously in the background, updating uptime metrics and profiler stats even when user programs are blocked on I/O.
- **Priority Classes**: Supports real-time, high, normal, and idle priority queues.

---

## 2. Process Lifecycle & Management

Processes are created using `SYS_spawn` or `SYS_termspawn`:
- **Parent/Child Hierarchy**: Each process has a PID, parent PID (PPID), and process group (PGID).
- **Background Execution**: In the shell, appending `&` to any command (e.g. `sleep 10 &`) runs the process in the background. The shell automatically reaps terminated children via `SYS_reapany`.
- **Process Inspection**:
  - `ps` lists all active processes, priority, state, ring, and memory footprint.
  - `top` provides a real-time updating terminal monitor with CPU usage bars.
  - `taskman` launches a GUI task manager with graphical CPU bars and a kill button.

---

## 3. Inter-Process Communication (IPC)

PlusOS provides a full suite of synchronization and communication primitives:

### FIFO Pipes (`kernel/proc/pipe.c`)
- Unidirectional byte streams with circular ring buffers.
- Supports non-blocking and blocking reads/writes with automatic sleep/wake on full/empty buffers (`SYS_pipe_create`, `SYS_pipe_read`, `SYS_pipe_write`, `SYS_pipe_close`).

### Semaphores & Mutexes (`kernel/proc/sync.c`)
- Counting semaphores (`SYS_sem_init`, `SYS_sem_wait`, `SYS_sem_signal`) for resource arbitration.
- Reentrant mutual exclusion locks (`SYS_mutex_lock`, `SYS_mutex_unlock`) with priority inheritance to prevent priority inversion.

### Asynchronous Signals (`kernel/proc/signal.c`)
- POSIX-style signals including `SIGKILL` (9), `SIGTERM` (15), `SIGINT` (2), `SIGUSR1` (10), and `SIGALRM` (14).
- Processes can register custom signal handlers or use default kernel dispositions (`SYS_signal_send`, `SYS_signal_handle`).

### Shared Memory (`kernel/proc/shm.c`)
- High-speed zero-copy memory sharing between processes (`SYS_shm_create`, `SYS_shm_map`, `SYS_shm_unmap`). Perfect for passing audio buffers and image surfaces directly to the Desktop compositor.

---

## Navigation

- **Previous Chapter**: [← 05. Dynamic RAM Management & Garbage Collection](/wiki/05-memory-management-and-gc)
- **Next Chapter**: [07. Desktop GUI & Multi-OS Theme Engine →](/wiki/07-desktop-and-window-server)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)
