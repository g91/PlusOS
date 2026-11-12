/*
 * lc3_color_quickdraw.h - Color QuickDraw Types and Function Declarations
 */

#ifndef LC3_COLOR_QUICKDRAW_H
#define LC3_COLOR_QUICKDRAW_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} RGBColor;

typedef struct {
    int16_t  value;
    RGBColor rgb;
} ColorSpec;

typedef struct {
    int32_t   ctSeed;
    int16_t   ctFlags;
    int16_t   ctSize;
    ColorSpec ctTable[256];
} ColorTable, *CTabPtr, **CTabHandle;

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} Rect;

typedef struct {
    uint8_t   *baseAddr;
    int16_t    rowBytes;
    Rect       bounds;
    int16_t    pmVersion;
    int16_t    packType;
    int32_t    packSize;
    int32_t    hRes;
    int32_t    vRes;
    int16_t    pixelType;
    int16_t    pixelSize;
    int16_t    cmpCount;
    int16_t    cmpSize;
    int32_t    planeBytes;
    CTabHandle pmTable;
    int32_t    pmReserved;
} PixMap, *PixMapPtr, **PixMapHandle;

typedef struct GDevice {
    int16_t         gdRefNum;
    int16_t         gdID;
    int16_t         gdType;
    int16_t         gdITable;
    int16_t         gdResPref;
    int32_t         gdSearchProc;
    int32_t         gdCompProc;
    int16_t         gdFlags;
    PixMapHandle    gdPMap;
    int32_t         gdRefCon;
    struct GDevice **gdNextGD;
    Rect            gdRect;
    int16_t         gdMode;
    int16_t         gdCCBytes;
    int16_t         gdCCDepth;
    int32_t         gdCCXData;
    int32_t         gdCCXMask;
    int32_t         gdReserved;
} GDevice, *GDPtr, **GDHandle;

PixMapHandle lc3_NewPixMap(void);
void lc3_DisposPixMap(PixMapHandle pm);
void lc3_CopyPixMap(PixMapHandle src, PixMapHandle dst);
void lc3_SetGDevice(GDHandle gdh);
GDHandle lc3_GetGDevice(void);
void lc3_DeviceLoop(void *rgn, void (*drawingProc)(int16_t depth, int16_t flags, GDHandle targetDevice, int32_t userData), int32_t userData, uint32_t flags);

#endif /* LC3_COLOR_QUICKDRAW_H */
