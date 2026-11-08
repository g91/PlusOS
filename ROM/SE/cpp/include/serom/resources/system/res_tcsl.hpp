#pragma once
#ifndef SEROM_RESOURCES_RES_TCSL_HPP
#define SEROM_RESOURCES_RES_TCSL_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/resources/res_tcsl.s  (9 entry points)
namespace serom { namespace resources { namespace res_tcsl { 
    void Resource_tcsl_0000();
    void Resource_tcsl_0000_Header();
    void _tcsl_0_();
    void TCSL_SLIDESHOW();
    void TCSL_NEXTFRAME();
    void TCSL_LOADBBMC();
    void TCSL_DRAWFRAME();
    void TCSL_ROWLOOP();
    void TCSL_NOBBMC();
}}}  // namespace serom::resources::res_tcsl
#endif  // SEROM_RESOURCES_RES_TCSL_HPP
