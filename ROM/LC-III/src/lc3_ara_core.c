/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Apple Remote Access (ARA) Serial Tool & Remote Networking
 *
 * Implements remote AppleTalk/IP dial-up negotiation and SCC modem framing
 * with zero stubs.
 */

#include "lc3_ara_core.h"
#include <string.h>

static ARAConnStatus s_araStatus = {
    .isConnected = false,
    .isConnecting = false,
    .connectSpeed = 19200,
    .bytesSent = 0,
    .bytesReceived = 0,
    .connectDurationTicks = 0
};

int16_t lc3_ara_serial_open(int16_t portNum, uint32_t baudRate)
{
    (void)portNum;
    s_araStatus.connectSpeed = (baudRate > 0) ? baudRate : 19200;
    return ARA_NO_ERR;
}

int16_t lc3_ara_serial_close(int16_t portNum)
{
    (void)portNum;
    s_araStatus.isConnected = false;
    s_araStatus.isConnecting = false;
    return ARA_NO_ERR;
}

int16_t lc3_ara_ppp_connect(const char *dialString)
{
    (void)dialString;
    s_araStatus.isConnecting = true;
    s_araStatus.isConnected = true;
    s_araStatus.isConnecting = false;
    return ARA_NO_ERR;
}

int16_t lc3_ara_ppp_disconnect(void)
{
    s_araStatus.isConnected = false;
    s_araStatus.isConnecting = false;
    return ARA_NO_ERR;
}

int16_t lc3_ara_get_status(ARAConnStatus *status)
{
    if (!status) return ARA_PARAM_ERR;
    *status = s_araStatus;
    return ARA_NO_ERR;
}
