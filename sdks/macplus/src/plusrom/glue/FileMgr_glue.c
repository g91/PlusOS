/* FileMgr_glue.c - the classic high-level File Manager calls.
 *
 * On a real Mac these lived in Apple's interface library, not in ROM: each
 * fills a parameter block and calls a ROM trap (PBOpen, PBRead, ...). The
 * PlusOS SDK provides them so code written against Inside Macintosh works
 * unchanged. sdk/tools/gen_sdk.py marks exactly these as available.
 */
#include <plusrom/FileMgr.h>
#include <string.h>

static void zero(void *p, size_t n) { memset(p, 0, n); }

pascal OSErr FSOpen(ConstStringPtr filen, INTEGER vrn, INTEGER *rn)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.ioParam.ioNamePtr = (StringPtr)filen;
    pb.ioParam.ioVRefNum = vrn;
    pb.ioParam.ioPermssn = 0;                 /* fsCurPerm */
    OSErr e = PBOpenSync(&pb);
    *rn = pb.ioParam.ioRefNum;
    return e;
}

pascal OSErr OpenRF(ConstStringPtr filen, INTEGER vrn, INTEGER *rn)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.ioParam.ioNamePtr = (StringPtr)filen;
    pb.ioParam.ioVRefNum = vrn;
    OSErr e = PBOpenRFSync(&pb);
    *rn = pb.ioParam.ioRefNum;
    return e;
}

pascal OSErr FSRead(INTEGER rn, LONGINT *count, void *buffp)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.ioParam.ioRefNum = rn;
    pb.ioParam.ioBuffer = buffp;
    pb.ioParam.ioReqCount = *count;
    pb.ioParam.ioPosMode = 0;                 /* fsAtMark */
    OSErr e = PBReadSync(&pb);
    *count = pb.ioParam.ioActCount;
    return e;
}

pascal OSErr FSWrite(INTEGER rn, LONGINT *count, const void *buffp)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.ioParam.ioRefNum = rn;
    pb.ioParam.ioBuffer = (Ptr)buffp;
    pb.ioParam.ioReqCount = *count;
    pb.ioParam.ioPosMode = 0;
    OSErr e = PBWriteSync(&pb);
    *count = pb.ioParam.ioActCount;
    return e;
}

pascal OSErr GetFPos(INTEGER rn, LONGINT *filep)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.ioParam.ioRefNum = rn;
    OSErr e = PBGetFPosSync(&pb);
    *filep = pb.ioParam.ioPosOffset;
    return e;
}

pascal OSErr SetFPos(INTEGER rn, INTEGER posmode, LONGINT possoff)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.ioParam.ioRefNum = rn;
    pb.ioParam.ioPosMode = posmode;
    pb.ioParam.ioPosOffset = possoff;
    return PBSetFPosSync(&pb);
}

pascal OSErr GetEOF(INTEGER rn, LONGINT *eof)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.ioParam.ioRefNum = rn;
    OSErr e = PBGetEOFSync(&pb);
    *eof = (LONGINT)pb.ioParam.ioMisc;
    return e;
}

pascal OSErr SetEOF(INTEGER rn, LONGINT eof)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.ioParam.ioRefNum = rn;
    pb.ioParam.ioMisc = (IOMiscType)eof;
    return PBSetEOFSync(&pb);
}

pascal OSErr FSClose(INTEGER rn)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.ioParam.ioRefNum = rn;
    return PBCloseSync(&pb);
}

pascal OSErr Create(ConstStringPtr filen, INTEGER vrn, OSType creator, OSType filtyp)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.fileParam.ioNamePtr = (StringPtr)filen;
    pb.fileParam.ioVRefNum = vrn;
    OSErr e = PBCreateSync(&pb);
    if (e) return e;
    zero(&pb, sizeof pb);
    pb.fileParam.ioNamePtr = (StringPtr)filen;
    pb.fileParam.ioVRefNum = vrn;
    e = PBGetFInfoSync(&pb);
    if (e) return e;
    pb.fileParam.ioFlFndrInfo.fdCreator = creator;
    pb.fileParam.ioFlFndrInfo.fdType = filtyp;
    return PBSetFInfoSync(&pb);
}

pascal OSErr FSDelete(ConstStringPtr filen, INTEGER vrn)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.fileParam.ioNamePtr = (StringPtr)filen;
    pb.fileParam.ioVRefNum = vrn;
    return PBDeleteSync(&pb);
}

pascal OSErr GetFInfo(ConstStringPtr filen, INTEGER vrn, FInfo *fndrinfo)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.fileParam.ioNamePtr = (StringPtr)filen;
    pb.fileParam.ioVRefNum = vrn;
    OSErr e = PBGetFInfoSync(&pb);
    *fndrinfo = pb.fileParam.ioFlFndrInfo;
    return e;
}

pascal OSErr SetFInfo(ConstStringPtr filen, INTEGER vrn, FInfo *fndrinfo)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.fileParam.ioNamePtr = (StringPtr)filen;
    pb.fileParam.ioVRefNum = vrn;
    OSErr e = PBGetFInfoSync(&pb);
    if (e) return e;
    pb.fileParam.ioFlFndrInfo = *fndrinfo;
    return PBSetFInfoSync(&pb);
}

pascal OSErr Rename(ConstStringPtr filen, INTEGER vrn, ConstStringPtr newf)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.ioParam.ioNamePtr = (StringPtr)filen;
    pb.ioParam.ioVRefNum = vrn;
    pb.ioParam.ioMisc = (IOMiscType)newf;
    return PBRenameSync(&pb);
}

pascal OSErr GetVol(StringPtr voln, INTEGER *vrn)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.volumeParam.ioNamePtr = voln;
    OSErr e = PBGetVolSync(&pb);
    *vrn = pb.volumeParam.ioVRefNum;
    return e;
}

pascal OSErr SetVol(ConstStringPtr voln, INTEGER vrn)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.volumeParam.ioNamePtr = (StringPtr)voln;
    pb.volumeParam.ioVRefNum = vrn;
    return PBSetVolSync(&pb);
}

pascal OSErr FlushVol(ConstStringPtr voln, INTEGER vrn)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.volumeParam.ioNamePtr = (StringPtr)voln;
    pb.volumeParam.ioVRefNum = vrn;
    return PBFlushVolSync(&pb);
}

pascal OSErr Eject(ConstStringPtr voln, INTEGER vrn)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.volumeParam.ioNamePtr = (StringPtr)voln;
    pb.volumeParam.ioVRefNum = vrn;
    return PBEject(&pb);
}

pascal OSErr GetVInfo(INTEGER drv, StringPtr voln, INTEGER *vrn, LONGINT *freeb)
{
    ParamBlockRec pb; zero(&pb, sizeof pb);
    pb.volumeParam.ioNamePtr = voln;
    pb.volumeParam.ioVRefNum = drv;
    OSErr e = PBGetVInfoSync(&pb);
    *vrn = pb.volumeParam.ioVRefNum;
    *freeb = (LONGINT)pb.volumeParam.ioVFrBlk * pb.volumeParam.ioVAlBlkSiz;
    return e;
}
