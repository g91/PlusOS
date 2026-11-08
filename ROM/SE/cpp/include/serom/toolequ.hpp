#pragma once
#ifndef SEROM_TOOLEQU_HPP
#define SEROM_TOOLEQU_HPP
//
// serom/toolequ.hpp -- Macintosh Toolbox record structures
// Transcribed from F:\mac_rom\include\toolequ.inc
//
#include "serom/machine.hpp"

namespace serom {
namespace toolbox {

struct Point {
    SInt16 v; // vertical coordinate (y)
    SInt16 h; // horizontal coordinate (x)
};

struct Rect {
    SInt16 top;
    SInt16 left;
    SInt16 bottom;
    SInt16 right;
};

struct Pattern {
    UInt8 pat[8];
};

struct BitMap {
    Addr   baseAddr; // pointer to pixel data
    SInt16 rowBytes; // bytes per scanline
    Rect   bounds;   // bounding box
};

struct GrafPort {
    SInt16   device;
    BitMap   portBits;
    Rect     portRect;
    Addr     visRgn;    // Handle to Region
    Addr     clipRgn;   // Handle to Region
    Pattern  bkPat;
    Pattern  fillPat;
    Point    pnLoc;
    Point    pnSize;
    SInt16   pnMode;
    Pattern  pnPat;
    SInt16   pnVis;
    SInt16   txFont;
    UInt8    txFace;
    UInt8    txMode;
    SInt16   txSize;
    UInt32   spExtra;
    UInt32   fgColor;
    UInt32   bkColor;
    SInt16   colrBit;
    SInt16   patStretch;
    Addr     picSave;
    Addr     rgnSave;
    Addr     polySave;
    Addr     grafProcs;
};

struct WindowRecord {
    GrafPort port;
    SInt16   windowKind;
    UInt8    visible;
    UInt8    hilited;
    UInt8    goAwayFlag;
    UInt8    spareFlag;
    Addr     strucRgn;
    Addr     contRgn;
    Addr     updateRgn;
    Addr     windowDefProc;
    Addr     dataHandle;
    Addr     titleHandle;
    SInt16   titleWidth;
    Addr     controlList;
    Addr     nextWindow;
    Addr     windowPic;
    SInt32   refCon;
};

struct MenuRecord {
    SInt16   menuID;
    SInt16   menuWidth;
    SInt16   menuHeight;
    Addr     menuProc;
    UInt32   enableFlags;
    UInt8    menuData[256];
};

struct ControlRecord {
    Addr     nextControl;
    Addr     contrlOwner;
    Rect     contrlRect;
    UInt8    contrlVis;
    UInt8    contrlHilite;
    SInt16   contrlValue;
    SInt16   contrlMin;
    SInt16   contrlMax;
    Addr     contrlDefProc;
    Addr     contrlData;
    Addr     contrlAction;
    SInt32   contrlRfCon;
    UInt8    contrlTitle[256];
};

} // namespace toolbox
} // namespace serom

#endif // SEROM_TOOLEQU_HPP
