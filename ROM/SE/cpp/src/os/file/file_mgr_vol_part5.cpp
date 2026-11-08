#include "serom/os/file/file_mgr_vol.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: file_mgr_vol
namespace serom { namespace os { namespace file_mgr_vol { 
    void HFSE_OK_13() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void HFSE_COPY_VOLNAME_DONE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void HFSE_CONT_14() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void HFSE_NEXT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void HFSE_CONT_15() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void HFSE_CONT_16() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void HFSE_ERR_9() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void HFSE_OK_14() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
}}}  // file_mgr_vol
