#pragma once
#ifndef SEROM_RESOURCES_RES_BBMC_HPP
#define SEROM_RESOURCES_RES_BBMC_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/resources/res_bbmc.s  (11 entry points)
namespace serom { namespace resources { namespace res_bbmc { 
    void Resource_bbmc_0000();
    void Resource_bbmc_0_Header();
    void Resource_bbmc_0000_Header();
    void bbmc_team_photos();
    void Resource_bbmc_0_FrameOffsetTable();
    void _bbmc_0_();
    void BBMC_Frame_1_PackBits();
    void BBMC_Frame_2_PackBits();
    void U();
    void BBMC_Frame_3_PackBits();
    void BBMC_Frame_4_PackBits();
}}}  // namespace serom::resources::res_bbmc
#endif  // SEROM_RESOURCES_RES_BBMC_HPP
