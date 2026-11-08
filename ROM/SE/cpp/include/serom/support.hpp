#pragma once
#ifndef SEROM_SUPPORT_HPP
#define SEROM_SUPPORT_HPP
// Macintosh SE ROM C++ port -- shared support.
//
// Pulls in the runtime foundation (68000 register/ABI model, low-memory globals,
// and Memory Manager structures) that the ported routines build on.
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"

namespace serom {
// Single chokepoint for every not-yet-ported ROM routine.  A ported routine must
// no longer reach this; it is an explicit marker of remaining work, not a no-op.
void serom_unimplemented(const char* which);
}
#endif // SEROM_SUPPORT_HPP
