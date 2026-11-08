#pragma once
#ifndef SEROM_RESOURCES_RES_MAP_HPP
#define SEROM_RESOURCES_RES_MAP_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/resources/res_map.s  (2 entry points)
namespace serom { namespace resources { namespace res_map { 
    void ROM_Resource_Area_Header();
    void ROM_Resource_Map();
}}}  // namespace serom::resources::res_map
#endif  // SEROM_RESOURCES_RES_MAP_HPP
