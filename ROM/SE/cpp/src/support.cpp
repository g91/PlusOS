#include "serom/support.hpp"
namespace serom {

// The one 68000 machine register context, shared by all ported routines as the
// register-based ABI (see serom/machine.hpp).
Regs g_cpu = {};

void serom_unimplemented(const char* which) {
    (void)which;  // not-yet-ported ROM routine
}

}
