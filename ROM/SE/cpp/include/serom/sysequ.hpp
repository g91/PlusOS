#pragma once
#ifndef SEROM_SYSEQU_HPP
#define SEROM_SYSEQU_HPP
//
// serom/sysequ.hpp -- Macintosh System Equates and I/O records
// Transcribed from F:\mac_rom\include\sysequ.inc
//
#include "serom/machine.hpp"
#include "serom/toolequ.hpp"

namespace serom {
namespace system {

// Queue Header (10 bytes)
struct QHdr {
    UInt16 qFlags;
    Addr   qHead;
    Addr   qTail;
};

// Queue Element header (first long is qLink)
struct QElem {
    Addr   qLink;
    SInt16 qType;
};

// Event Record (16 bytes)
struct EventRecord {
    UInt16 what;      // event code
    UInt32 message;   // event message (e.g. key char/code or window pointer)
    UInt32 when;      // tick count
    toolbox::Point where; // mouse location
    UInt16 modifiers; // modifier keys (shift, cmd, option, etc.)
};

// Standard I/O Parameter Block
struct IOParam {
    Addr   qLink;
    SInt16 qType;
    SInt16 ioTrap;
    Addr   ioCmdAddr;
    Addr   ioCompletion;
    SInt16 ioResult;
    Addr   ioNamePtr;
    SInt16 ioVRefNum;
    SInt16 ioRefNum;
    SInt8  ioPermssn;
    SInt8  filler;
    Addr   ioBuffer;
    SInt32 ioReqCount;
    SInt32 ioActCount;
    SInt16 ioPosMode;
    SInt32 ioPosOffset;
};

// Device Driver Header
struct DrvrHeader {
    UInt16 drvrFlags;
    UInt16 drvrDelay;
    UInt16 drvrEMask;
    UInt16 drvrMenu;
    UInt16 drvrOpen;
    UInt16 drvrPrime;
    UInt16 drvrCtl;
    UInt16 drvrStatus;
    UInt16 drvrClose;
    UInt8  drvrName[32];
};

} // namespace system
} // namespace serom

#endif // SEROM_SYSEQU_HPP
