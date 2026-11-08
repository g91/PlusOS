#pragma once
#ifndef SEROM_OS_VBL_MGR_HPP
#define SEROM_OS_VBL_MGR_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/os/vbl_mgr.s  (8 entry points)
namespace serom { namespace os { namespace vbl_mgr { 
    void VBLCORE();
    void VINSTALL();
    void VINSTALL_TYPERR();
    void VINSTALL_SYNC();
    void VINSTALL_PHASE_OK();
    void VINSTALL_ENQUEUE();
    void VREMOVE();
    void CORESTUB();
}}}  // namespace serom::os::vbl_mgr
#endif  // SEROM_OS_VBL_MGR_HPP
