#pragma once
// Macintosh SE ROM C++ scaffold -- shared support.
namespace serom {
// Single chokepoint for every not-yet-ported ROM routine.
void serom_unimplemented(const char* which);
}
