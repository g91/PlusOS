#pragma once
#ifndef SEROM_TOOLBOX_PACKAGE_MGR_HPP
#define SEROM_TOOLBOX_PACKAGE_MGR_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/package_mgr.s  (27 entry points)
namespace serom { namespace toolbox { namespace package_mgr { 
    void FMGREND();
    void PACKDISPATCHER();
    void INITPACK();
    void INITALLPACKS();
    void INITALL_PUSH_LOOP();
    void SANE_HALT_HANDLER();
    void MAP_PACK_SLOT();
    void MAP_PACK_INDEXED();
    void PACK_DISPATCH();
    void PACKDISP_LOADED();
    void PACKDISP_FAIL();
    void PACK0();
    void PACK1();
    void PACK2();
    void PACK3();
    void PACK4();
    void PACK5();
    void PACK6();
    void PACK7();
    void PACK8();
    void PACK9();
    void PACK10();
    void PACK11();
    void PACK12();
    void PACK13();
    void PACK14();
    void PACK15();
}}}  // namespace serom::toolbox::package_mgr
#endif  // SEROM_TOOLBOX_PACKAGE_MGR_HPP
