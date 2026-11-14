# Classic Mac 'APPL' Executable Loader

One of PlusOS's crowning technical achievements is its ability to run authentic Classic Macintosh applications (`'APPL'`) without running Mac OS.

---

## 1. The Challenge of Classic Mac Applications

Unlike modern operating systems or Unix binaries where code lives in the file's primary data fork, Classic Macintosh software was fundamentally designed around Apple's **Resource Fork** and the **Segment Loader**:
1. Executable code is split across `'CODE'` resources.
2. Applications rely on the **A5 World**: a specific memory structure anchored to register `A5` holding QuickDraw globals, application variables, and the **Jump Table**.
3. Procedure calls across code segments jump through Jump Table stubs that invoke `_LoadSeg` (`$A9F0`) to page segments in from disk on demand.
4. Program exit calls `_ExitToShell` (`$A9F4`).

---

## 2. The PlusOS MacLoader Engine (`kernel/macloader.c`)

When a user launches an application with `macrun <file>` or double-clicks it in the Desktop file manager, `macloader.c` performs the following steps:

### Container Detection
Inspects the target file to determine container format:
- **HFS Resource Fork**: Accessed directly via ROM `OpenRF`.
- **Raw Resource Fork (`.rsrc`)**: Parsed in-place.
- **MacBinary I/II/III (`.bin`)**: Decodes the 128-byte header, extracts Finder type (`'APPL'`), and offsets into the embedded resource fork.

### CODE 0 & Jump Table Synthesis
Reads `'CODE'` 0, which defines the memory layout:
- `aboveA5`: Bytes required above `A5` for QuickDraw and the Jump Table.
- `belowA5`: Bytes required below `A5` for application globals.
- `jtSize` & `jtOffset`: Byte size and offset to the initial Jump Table entries.

### A5 World Allocation
Allocates memory and constructs the A5 World:
- Points low-memory global `CurrentA5` (`$0904`) to the boundary.
- Copies the Jump Table from `'CODE'` 0 into `A5 + jtOffset`.
- Injects the QuickDraw globals pointer at `0(A5)`.

### Dynamic Trap Hooking
Hooks key ROM traps via the vector table:
- **`_LoadSeg` (`$A9F0`)**: Intercepted so when an un-loaded segment is called, PlusOS reads the `'CODE'` resource from the resource fork, locks it in memory, rewrites the Jump Table entry to point directly to the loaded code, and branches to it!
- **`_UnLoadSeg` (`$A9F1`)**: Releases loaded code resources when memory pressure occurs.
- **`_ExitToShell` (`$A9F4`)**: Caught cleanly by PlusOS to free all process resources, restore trap vectors, and return smoothly to the Desktop or Shell without rebooting!

---

## Navigation

- **Previous Chapter**: [← 07. Desktop GUI & Multi-OS Theme Engine](/wiki/07-desktop-and-window-server)
- **Next Chapter**: [09. PlusNET: DaynaPORT TCP/IP Stack →](/wiki/09-networking-plusnet)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)
