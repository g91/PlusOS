#pragma once
#ifndef SEROM_BOOT_VECTORS_HPP
#define SEROM_BOOT_VECTORS_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/boot/vectors.s  (11 entry points)
namespace serom { namespace boot { namespace vectors { 
    void MYFIRSTPROC();
    void MYROM();
    void BASEOFROM();
    void STARTPC();
    void ROMVERSION();
    void STBOOT();
    void BADDISK();
    void ROMRSRC();
    void DISPOFF();
    void CRITICAL();
    void RESETENTRY();
}}}  // namespace serom::boot::vectors
#endif  // SEROM_BOOT_VECTORS_HPP
