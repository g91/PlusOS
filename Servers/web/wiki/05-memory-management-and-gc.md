# Dynamic RAM Management & Garbage Collection

PlusOS 3.1 features an advanced, multi-tier memory management architecture designed to maximize stability and prevent memory fragmentation on RAM-constrained 68000 systems.

---

## Architecture Overview

```text
+-------------------------------------------------------------+
|                     User Application                        |
+------------------------------+------------------------------+
                               |
            SYS_kmem_alloc / SYS_kmem_free
                               v
+-------------------------------------------------------------+
|         Fast Slab Allocators (32, 64, 128, 256, 512 Bytes)  |
|         Fixed-size object pools, zero internal fragmentation|
+------------------------------+------------------------------+
                               | Fallback (requests > 512B)
                               v
+-------------------------------------------------------------+
|          Dynamic Free-List Heap Manager (`kmem.c`)          |
|          Best-fit search, header canaries, boundary tags    |
+------------------------------+------------------------------+
                               |
            Out-of-memory or high fragmentation threshold
                               v
+-------------------------------------------------------------+
|          Mark-Sweep Garbage Collector & Compactor (`gc.c`)  |
|          Compacts movable handles, coalesces free blocks    |
+-------------------------------------------------------------+
```

---

## 1. Fast Slab Allocators

For small, high-frequency kernel allocations (such as IPC message packets, task control blocks, file handles, and window event nodes), PlusOS provides five fixed-size **slab pools**:
- **32 Bytes**: Event queue nodes and timer callbacks.
- **64 Bytes**: IPC message headers and file descriptors.
- **128 Bytes**: Task control metadata and environment variables.
- **256 Bytes**: Path buffers and networking packets.
- **512 Bytes**: Disk cache sectors and terminal scanline buffers.

Allocations from slab pools take $O(1)$ time and produce zero memory fragmentation.

---

## 2. Dynamic Heap (`kmem.c`) & Sentinel Canaries

Allocations larger than 512 bytes are serviced by the kernel dynamic heap:
- **Best-fit block selection**: Finds the smallest available chunk that fits the request, preserving large contiguous spans for program loading.
- **Canary Sentinels**: Each allocated block is prefixed with a 4-byte magic canary (`0x504C4F53` - "PLOS") and followed by a trailer canary. The memory manager verifies canaries during every `free()` and `realloc()` call, instantly catching buffer overruns and memory corruptions before they cause system panics.

---

## 3. Automatic Garbage Collector & Compactor (`gc.c`)

When free memory drops below a configurable threshold, the background **Garbage Collector** triggers automatically:
1. **Mark Phase**: Traverses active process roots, thread stacks, open file tables, and IPC queues, marking reachable memory chunks.
2. **Sweep Phase**: Reclaims unreachable and orphaned memory blocks allocated by terminated processes.
3. **Compaction Phase**: Coalesces adjacent free blocks and slides relocatable memory handles down toward lower addresses, combating external fragmentation.

Users can view memory health and run manual collections using the CLI tools:
```text
ram       # Displays dynamic heap statistics, slab utilization, and largest free span
gc        # Triggers an immediate sweep and heap compaction
```

---

## 4. Virtual RAM Disk (`ramdisk.c`)

PlusOS provides an ultra-fast in-memory RAM disk (`SYS_ramdisk_read` / `SYS_ramdisk_write`):
- Memory is lazily allocated on first use so low-RAM systems do not lose valuable heap space at boot.
- Offers instantaneous temporary storage for shell scripts, scratch files, and rapid inter-process data piping.

---

## Navigation

- **Previous Chapter**: [← 04. Kernel Core & Dual-Ring Execution](/wiki/04-kernel-architecture)
- **Next Chapter**: [06. Preemptive Multitasking & IPC Subsystems →](/wiki/06-multitasking-and-ipc)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)
