#pragma once
#ifndef SEROM_TOOLBOX_DESK_MGR_HPP
#define SEROM_TOOLBOX_DESK_MGR_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/desk_mgr.s  (38 entry points)
namespace serom { namespace toolbox { namespace desk_mgr { 
    void DESKMGR();
    void SYSTEMEVENT();
    void SYSEVT_DISPATCH();
    void SYSEVT_HANDLED();
    void SYSEVT_RESTORE();
    void TEXTEDIT_CONT();
    void TEXTEDIT_CONT_2();
    void TEXTEDIT_ERR();
    void TEXTEDIT_CONT_3();
    void TEXTEDIT_CONT_4();
    void TEXTEDIT_CONT_5();
    void TEXTEDIT_CONT_6();
    void DISKINSERT_ENTRY();
    void TEXTEDIT_CONT_7();
    void SYSTEMCLICK();
    void SYSTEMCLICK_EXIT();
    void SYSTEMCLICK_EMPTY();
    void SYSTEMCLICK_NO();
    void SYSTEMCLICK_OK();
    void SYSTEMCLICK_OK_2();
    void SYSTEMCLICK_OK_3();
    void SYSTEMTASK();
    void SYSTEMTASK_SKIP();
    void SYSTEMTASK_LOOP();
    void SYSTEMTASK_RUN_DA();
    void SYSTEMTASK_SKIP_2();
    void SYSTEMTASK_CONT();
    void SYSTEMMENU();
    void SYSTEMMENU_EMPTY();
    void SYSTEMMENU_LOOP();
    void SYSTEMMENU_SKIP();
    void SYSTEMMENU_DONE();
    void SYSTEMEDIT();
    void SYSEDIT_SKIP();
    void OPENDESKACC();
    void OPENDESKACC_ERR();
    void CLOSEDESKACC();
    void CLOSEDESKACC_EXIT();
}}}  // namespace serom::toolbox::desk_mgr
#endif  // SEROM_TOOLBOX_DESK_MGR_HPP
