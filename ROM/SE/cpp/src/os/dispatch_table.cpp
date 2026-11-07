#include "serom/os/dispatch_table.hpp"
// Scaffold for SE ROM module: se/os/dispatch_table.s
namespace serom { namespace os { namespace dispatch_table { 
    void RESTOP() { serom::serom_unimplemented("os::dispatch_table::RESTOP"); }
    void DSPTABLE() { serom::serom_unimplemented("os::dispatch_table::DSPTABLE"); }
    void Compressed_ALine_Dispatch_Table() { serom::serom_unimplemented("os::dispatch_table::Compressed_ALine_Dispatch_Table"); }
    void DISPTABLE() { serom::serom_unimplemented("os::dispatch_table::DISPTABLE"); }
}}}  // namespace serom::os::dispatch_table
