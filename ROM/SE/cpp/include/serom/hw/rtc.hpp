#pragma once
#ifndef SEROM_HW_RTC_HPP
#define SEROM_HW_RTC_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/hw/rtc.s  (25 entry points)
namespace serom { namespace hw { namespace rtc { 
    void CLOCK();
    void CLKRW();
    void CLKRW_EXIT();
    void CLKRW_CONT();
    void READXPRAM();
    void WRITEXPRAM();
    void WRITEXPRAM_CONT();
    void WRITEXPRAM_LOOP();
    void WRITEXPRAM_LOOP_2();
    void WRITEXPRAM_LOOP_3();
    void WRITEXPRAM_EXIT();
    void WRITEXPRAM_ERR();
    void CLKNOMEM();
    void RTC_EXT_CMD_SENT();
    void RTC_SINGLE_CMD();
    void RTC_READ_OR_WRITE();
    void RTC_SEND_DATA_BYTE();
    void RTC_WRITE_DONE();
    void RTC_READ_PATH();
    void RTC_READ_DATA_BYTE();
    void RTC_DISABLE_CHIP();
    void WriteRTCByte();
    void WRITE_RTC_BIT_LOOP();
    void ReadRTCByte();
    void READ_RTC_BIT_LOOP();
}}}  // namespace serom::hw::rtc
#endif  // SEROM_HW_RTC_HPP
