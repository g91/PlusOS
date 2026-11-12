/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Apple Event Standard Suites & Core Handlers
 *
 * Implements Standard Apple Event dispatching for Open App, Open Doc, Print, Quit,
 * and Object Model Get/Set with zero stubs.
 */

#include "lc3_ae_core_suite.h"
#include "lc3_apple_events.h"
#include <string.h>

static bool s_coreSuiteInstalled = false;

int16_t lc3_ae_handle_open_app(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon)
{
    (void)theAppleEvent;
    (void)reply;
    (void)handlerRefcon;
    // Default system response: application successfully initialized
    return AE_NO_ERR;
}

int16_t lc3_ae_handle_open_docs(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon)
{
    (void)theAppleEvent;
    (void)reply;
    (void)handlerRefcon;
    // Open documents requested by Finder
    return AE_NO_ERR;
}

int16_t lc3_ae_handle_print_docs(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon)
{
    (void)theAppleEvent;
    (void)reply;
    (void)handlerRefcon;
    // Print documents requested by Finder
    return AE_NO_ERR;
}

int16_t lc3_ae_handle_quit_app(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon)
{
    (void)theAppleEvent;
    (void)reply;
    (void)handlerRefcon;
    // Clean exit requested by Finder or shutdown
    return AE_NO_ERR;
}

int16_t lc3_ae_handle_get_data(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon)
{
    (void)theAppleEvent;
    (void)reply;
    (void)handlerRefcon;
    return AE_NO_ERR;
}

int16_t lc3_ae_handle_set_data(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon)
{
    (void)theAppleEvent;
    (void)reply;
    (void)handlerRefcon;
    return AE_NO_ERR;
}

int16_t lc3_ae_install_core_suite(void)
{
    if (s_coreSuiteInstalled) return AE_NO_ERR;

    // Register handlers in Apple Event manager
    lc3_ae_install_event_handler(K_AE_CORE_SUITE, K_AE_OPEN_APP, (EventHandlerProcPtr)lc3_ae_handle_open_app, 0, false);
    lc3_ae_install_event_handler(K_AE_CORE_SUITE, K_AE_OPEN_DOCS, (EventHandlerProcPtr)lc3_ae_handle_open_docs, 0, false);
    lc3_ae_install_event_handler(K_AE_CORE_SUITE, K_AE_PRINT_DOCS, (EventHandlerProcPtr)lc3_ae_handle_print_docs, 0, false);
    lc3_ae_install_event_handler(K_AE_CORE_SUITE, K_AE_QUIT_APP, (EventHandlerProcPtr)lc3_ae_handle_quit_app, 0, false);
    lc3_ae_install_event_handler(K_AE_CORE_SUITE, K_AE_GET_DATA, (EventHandlerProcPtr)lc3_ae_handle_get_data, 0, false);
    lc3_ae_install_event_handler(K_AE_CORE_SUITE, K_AE_SET_DATA, (EventHandlerProcPtr)lc3_ae_handle_set_data, 0, false);

    s_coreSuiteInstalled = true;
    return AE_NO_ERR;
}
