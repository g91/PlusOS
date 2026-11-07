#pragma once
#ifndef SEROM_TOOLBOX_PRINT_GLUE_HPP
#define SEROM_TOOLBOX_PRINT_GLUE_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/print_glue.s  (32 entry points)
namespace serom { namespace toolbox { namespace print_glue { 
    void PRGLUE();
    void PRG_FROM_DCE();
    void PRG_LOAD_PDEF();
    void PRG_GOT_PDEF();
    void PRG_PDEF_KEEP_LOCKED();
    void PRG_BUILTIN_DISPATCH();
    void PRGLUE_DISPATCH_TBL();
    void PrGlue_FAIL();
    void PRG_EXIT_NO_RESULT();
    void PrGlue_PROPEN();
    void PrGlue_PRCLOSE();
    void PRG_OPEN_RF_BODY();
    void PRG_GOT_NAME();
    void PRG_OPEN_DONE();
    void PRG_OPEN_FAIL();
    void PrGlue_OPEN();
    void PrGlue_CLOSE();
    void PrGlue_STATUS();
    void PrGlue_ATTR();
    void PrGlue_CONTROL();
    void PrGlue_PURGE();
    void PrGlue_NOPURGE();
    void PRG_STATUS_DCE();
    void PrGlue_GETERR();
    void PrGlue_PASSTHRU();
    void PRG_STORE_ERR();
    void PRG_EPILOGUE();
    void PRG_EPILOGUE_FRAME();
    void PRG_FRAME_RESTORE();
    void PRGLUE_GET_VREFNUM();
    void PRGLUE_SET_VOL();
    void PrintDriverName_Str();
}}}  // namespace serom::toolbox::print_glue
#endif  // SEROM_TOOLBOX_PRINT_GLUE_HPP
