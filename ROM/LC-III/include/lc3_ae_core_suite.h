/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Apple Event Standard Suites & Core Handlers
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_AE_CORE_SUITE_H
#define LC3_AE_CORE_SUITE_H

#include <stdint.h>
#include <stdbool.h>
#include "lc3_apple_events.h"

#define AE_NO_ERR             0

#define K_AE_CORE_SUITE       0x636F7265 // 'core'
#define K_AE_OPEN_APP         0x6F617070 // 'oapp'
#define K_AE_OPEN_DOCS        0x6F646F63 // 'odoc'
#define K_AE_PRINT_DOCS       0x70646F63 // 'pdoc'
#define K_AE_QUIT_APP         0x71756974 // 'quit'
#define K_AE_GET_DATA         0x67657464 // 'getd'
#define K_AE_SET_DATA         0x73657464 // 'setd'

/* Standard Core Handlers */
int16_t lc3_ae_handle_open_app(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon);
int16_t lc3_ae_handle_open_docs(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon);
int16_t lc3_ae_handle_print_docs(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon);
int16_t lc3_ae_handle_quit_app(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon);
int16_t lc3_ae_handle_get_data(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon);
int16_t lc3_ae_handle_set_data(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon);
int16_t lc3_ae_install_core_suite(void);

#endif /* LC3_AE_CORE_SUITE_H */
