#pragma once
#ifndef SEROM_TOOLBOX_GET_MGR_HPP
#define SEROM_TOOLBOX_GET_MGR_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/get_mgr.s  (19 entry points)
namespace serom { namespace toolbox { namespace get_mgr { 
    void GMGR();
    void GETPATTERN();
    void GETPATTERN_EXIT();
    void GETCURSOR();
    void GETSTRING();
    void GETICON();
    void GETPICTURE();
    void GETNEWWINDOW();
    void GETNEWWINDOW_FOUND();
    void GETNEWCONTROL();
    void GETNEWCONTROL_FOUND();
    void GETMENU();
    void GETRMENU_NO();
    void GETRMENU_NO_2();
    void GETRMENU_DONE();
    void GETRMENU_CONT();
    void GETNEWMBAR();
    void GETNEWMBAR_LOOP();
    void GETNEWMBAR_FOUND();
}}}  // namespace serom::toolbox::get_mgr
#endif  // SEROM_TOOLBOX_GET_MGR_HPP
