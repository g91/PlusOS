#pragma once
#ifndef SEROM_TOOLBOX_SCRAP_MGR_HPP
#define SEROM_TOOLBOX_SCRAP_MGR_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/scrap_mgr.s  (35 entry points)
namespace serom { namespace toolbox { namespace scrap_mgr { 
    void TE68KEND();
    void SCRAPMGR();
    void INFOSCRAP();
    void INFOSCRAP_RETURN();
    void SCRAP_SETUP();
    void SCRAP_LOAD_GLOBALS();
    void SCRAP_OPEN_FILE();
    void UNLOADSCRAP();
    void SCRAP_NOERR();
    void SCRAP_EXIT();
    void SCRAP_EXIT_STRIP();
    void SCRAP_RTS_OK();
    void SCRAP_RTS();
    void LOADSCRAP();
    void SCRAP_IOPARAM_HDL();
    void SCRAP_IOPARAM_A0();
    void ZEROSCRAP();
    void ZERO_CHK_STATE();
    void ZERO_RESIZE_HDL();
    void ZERO_STORE_HDL();
    void ZERO_TRUNCATE();
    void SCRAP_READ_DATA();
    void SCRAP_READ_FILE();
    void GETSCRAP();
    void GETSCRAP_SCAN();
    void GETSCRAP_NOTYPE();
    void GETSCRAP_FOUND();
    void GETSCRAP_ERR();
    void GETSCRAP_SET_ERR();
    void GETSCRAP_DONE();
    void SCRAP_ENSURE_INIT();
    void ENSUREINIT_RTS();
    void PUTSCRAP();
    void SCRAP_APPEND();
    void SCRAP_WRITE_FILE();
}}}  // namespace serom::toolbox::scrap_mgr
#endif  // SEROM_TOOLBOX_SCRAP_MGR_HPP
