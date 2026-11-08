#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
#include "serom/hwregs.hpp"
#include "serom/traps.hpp"
#include "serom/sysequ.hpp"
#include "serom/toolequ.hpp"
#include "serom/all.hpp"

extern "C" {

// Framebuffer base address for Mac SE (512x342, 1-bit)
static const serom::Addr kScreenBase = 0x003FA700ul;
static const int kScreenWidth = 512;
static const int kScreenHeight = 342;
static const int kRowBytes = 64;

// Classic Happy Mac Icon (32x32 bitmap)
static const serom::UInt32 kHappyMac[32] = {
    0x00000000, 0x07FFE000, 0x08001000, 0x10000800,
    0x10000800, 0x10000800, 0x10000800, 0x1381C800,
    0x1381C800, 0x1381C800, 0x10000800, 0x10000800,
    0x10180800, 0x10240800, 0x10420800, 0x10810800,
    0x10000800, 0x11008800, 0x10810800, 0x107E0800,
    0x10000800, 0x10000800, 0x10000800, 0x10000800,
    0x10000800, 0x10000800, 0x08001000, 0x07FFE000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000
};

// Main C++ ROM Bootstrap sequence
void serom_main_bootstrap() {
    using namespace serom;

    // 1. Initialize Low-Memory Globals
    lowmem::Set<UInt32>(lowmem::ROMBase, 0x00400000ul);
    lowmem::Set<UInt16>(lowmem::SysVersion, 0x0276);
    lowmem::Set<UInt32>(lowmem::MemTop, 0x00400000ul);       // 4 MB RAM
    lowmem::Set<UInt32>(lowmem::BufPtr, kScreenBase);        // Top of usable memory
    lowmem::Set<UInt32>(lowmem::HeapEnd, 0x00300000ul);
    lowmem::Set<UInt16>(lowmem::ScreenRow, kRowBytes);
    lowmem::Set<UInt32>(lowmem::SysZone, 0x00020000ul);      // System Heap zone
    lowmem::Set<UInt32>(lowmem::ApplZone, 0x00030000ul);     // Application Heap zone
    lowmem::Set<UInt32>(lowmem::TheZone, 0x00020000ul);
    lowmem::Set<UInt32>(lowmem::Ticks, 0);
    lowmem::Set<UInt32>(lowmem::Time, 2082844800ul);         // Classic epoch
    lowmem::Set<SInt16>(lowmem::BootDrive, 1);
    lowmem::Set<UInt16>(lowmem::MmInOK, 1);

    // 2. Initialize Memory Manager zones
    os::memory_mgr::INITZONE();
    os::memory_mgr::INITAPPLZONE();

    // 3. Paint Desktop Pattern (50% gray checkerboard: 0x55, 0xAA)
    for (int y = 0; y < kScreenHeight; ++y) {
        Addr row = kScreenBase + (y * kRowBytes);
        UInt8 pat = (y & 1) ? 0xAA : 0x55;
        for (int x = 0; x < kRowBytes; ++x) {
            PokeB(row + x, pat);
        }
    }

    // 4. Render Happy Mac in Center of Screen
    int iconX = (kScreenWidth / 2 - 16) / 8; // byte offset
    int iconY = (kScreenHeight / 2 - 16);
    for (int y = 0; y < 32; ++y) {
        Addr row = kScreenBase + ((iconY + y) * kRowBytes) + iconX;
        UInt32 bits = kHappyMac[y];
        PokeB(row + 0, static_cast<UInt8>(bits >> 24));
        PokeB(row + 1, static_cast<UInt8>(bits >> 16));
        PokeB(row + 2, static_cast<UInt8>(bits >> 8));
        PokeB(row + 3, static_cast<UInt8>(bits));
    }

    // 5. Initialize Core Subsystems
    boot::boot_load::INITEVENTS();
    toolbox::quickdraw::GrafAsm::INITGRAF();
    toolbox::window_mgr::INITWINDOWS();
    toolbox::menu_mgr::INITMENUS();
    toolbox::resource_mgr::INITRESOURCES();
}

// C++ Line 1010 A-Line Trap Dispatcher
void serom_dispatch_aline(void* frame, serom::UInt16 trapWord) {
    (void)frame;
    using namespace serom;

    // Check if OS Trap ($A000 - $A7FF) or Toolbox Trap ($A800 - $AFFF)
    if ((trapWord & 0x0800) == 0) {
        // OS Trap: pass through D0/A0
        UInt8 trapNum = trapWord & 0xFF;
        switch (trapNum) {
            case 0x1E: // _NewPtr
                os::memory_mgr::NEWPTR();
                break;
            case 0x1F: // _DisposePtr
                os::memory_mgr::DISPOSEPTR();
                break;
            case 0x21: // _GetPtrSize
                os::memory_mgr::GETPTRSIZE();
                break;
            case 0x20: // _SetPtrSize
                os::memory_mgr::SETPTRSIZE();
                break;
            case 0x22: // _NewHandle
                os::memory_mgr::NWHANDLE();
                break;
            case 0x23: // _DisposeHandle
                os::memory_mgr::DSPOSEHANDLE();
                break;
            case 0x25: // _GetHandleSize
                os::memory_mgr::GETHANDLESIZE();
                break;
            case 0x24: // _SetHandleSize
                os::memory_mgr::SETHANDLESIZE();
                break;
            case 0x29: // _HLock
                os::memory_mgr::HLOCK();
                break;
            case 0x2A: // _HUnlock
                os::memory_mgr::HUNLOCK();
                break;
            case 0x2F: // _PostEvent
                os::event_mgr::POSTEVENT();
                break;
            case 0x30: // _OSEventAvail
                os::event_mgr::OSEVENTAVAIL();
                break;
            case 0x31: // _GetOSEvent
                os::event_mgr::GETOSEVENT();
                break;
            case 0x32: // _FlushEvents
                os::event_mgr::FLUSHEVENTS();
                break;
            case 0x1C: // _FreeMem
                os::memory_mgr::FREEMEM();
                break;
            case 0x4C: // _CompactMem
                os::memory_mgr::COMPACTMEM();
                break;
            case 0x7C: // _ADBOp
                os::adb_mgr::ADBOP();
                break;
            default:
                D(0) = 0;
                SetNZ(0);
                break;
        }
    } else {
        // Toolbox Trap ($A800 - $AFFF)
        UInt16 toolNum = trapWord & 0x03FF;
        switch (toolNum) {
            case 0x06E: // _InitGraf
                toolbox::quickdraw::GrafAsm::INITGRAF();
                break;
            case 0x06F: // _OpenPort
                toolbox::quickdraw::GrafAsm::OPENPORT();
                break;
            case 0x073: // _SetPort
                toolbox::quickdraw::GrafAsm::SETPORT();
                break;
            case 0x074: // _GetPort
                toolbox::quickdraw::GrafAsm::GETPORT();
                break;
            case 0x0A7: // _SetRect
                toolbox::quickdraw::Rects::SETRECT();
                break;
            case 0x0A1: // _FrameRect
                toolbox::quickdraw::Rects::FRAMERECT();
                break;
            case 0x0A2: // _PaintRect
                toolbox::quickdraw::Rects::PAINTRECT();
                break;
            case 0x0A3: // _EraseRect
                toolbox::quickdraw::Rects::ERASERECT();
                break;
            case 0x0A4: // _InvertRect
                toolbox::quickdraw::Rects::INVERTRECT();
                break;
            case 0x0A5: // _FillRect
                toolbox::quickdraw::Rects::FILLRECT();
                break;
            case 0x0EC: // _CopyBits / _BitBlt
                toolbox::quickdraw::BitBlt::BITBLT();
                break;
            case 0x112: // _InitWindows
                toolbox::window_mgr::INITWINDOWS();
                break;
            case 0x113: // _NewWindow
                toolbox::window_mgr::NEWWINDOW();
                break;
            case 0x130: // _InitMenus
                toolbox::menu_mgr::INITMENUS();
                break;
            case 0x195: // _InitResources
                toolbox::resource_mgr::INITRESOURCES();
                break;
            default:
                D(0) = 0;
                SetNZ(0);
                break;
        }
    }
}

} // extern "C"
