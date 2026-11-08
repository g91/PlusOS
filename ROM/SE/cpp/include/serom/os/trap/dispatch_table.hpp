#pragma once
#ifndef SEROM_OS_DISPATCH_TABLE_HPP
#define SEROM_OS_DISPATCH_TABLE_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/os/dispatch_table.s  (4 entry points)
namespace serom { namespace os { namespace dispatch_table { 
    void RESTOP();
    void DSPTABLE();
    void Compressed_ALine_Dispatch_Table();
    void DISPTABLE();
}}}  // namespace serom::os::dispatch_table
#endif  // SEROM_OS_DISPATCH_TABLE_HPP
