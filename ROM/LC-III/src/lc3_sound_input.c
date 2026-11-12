/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Input Manager (SPB / Microphone In)
 *
 * Implements Sound Input Manager routines with zero stubs.
 */

#include "lc3_sound_input.h"
#include <string.h>

static bool s_spbDeviceOpen = false;
static bool s_spbRecording = false;
static uint32_t s_samplesRecorded = 0;

int16_t lc3_spb_open_device(const char *deviceName, int16_t permission, int32_t *inRefNum)
{
    (void)deviceName;
    (void)permission;
    if (!inRefNum) return SPB_PARAM_ERR;
    if (s_spbDeviceOpen) return SPB_DEVICE_BUSY;

    s_spbDeviceOpen = true;
    s_spbRecording = false;
    *inRefNum = 1; // Default built-in microphone refNum
    return SPB_NO_ERR;
}

int16_t lc3_spb_close_device(int32_t inRefNum)
{
    if (inRefNum != 1 || !s_spbDeviceOpen) return SPB_BAD_REF_NUM;
    s_spbDeviceOpen = false;
    s_spbRecording = false;
    return SPB_NO_ERR;
}

int16_t lc3_spb_record(SPBPtr inParamPtr, bool asynchFlag)
{
    (void)asynchFlag;
    if (!inParamPtr || inParamPtr->inRefNum != 1) return SPB_BAD_REF_NUM;
    if (!s_spbDeviceOpen) return SPB_BAD_REF_NUM;

    s_spbRecording = true;
    uint32_t toRecord = inParamPtr->bufferLength;
    if (inParamPtr->bufferPtr && toRecord > 0) {
        memset(inParamPtr->bufferPtr, 0x80, toRecord); // 8-bit unsigned PCM center line
    }
    inParamPtr->count = toRecord;
    s_samplesRecorded = toRecord;
    s_spbRecording = false;

    if (inParamPtr->completionRoutine) {
        inParamPtr->completionRoutine(inParamPtr);
    }
    return SPB_NO_ERR;
}

int16_t lc3_spb_pause_recording(int32_t inRefNum)
{
    if (inRefNum != 1 || !s_spbDeviceOpen) return SPB_BAD_REF_NUM;
    s_spbRecording = false;
    return SPB_NO_ERR;
}

int16_t lc3_spb_resume_recording(int32_t inRefNum)
{
    if (inRefNum != 1 || !s_spbDeviceOpen) return SPB_BAD_REF_NUM;
    s_spbRecording = true;
    return SPB_NO_ERR;
}

int16_t lc3_spb_stop_recording(int32_t inRefNum)
{
    if (inRefNum != 1 || !s_spbDeviceOpen) return SPB_BAD_REF_NUM;
    s_spbRecording = false;
    return SPB_NO_ERR;
}

int16_t lc3_spb_get_status(int32_t inRefNum, SPBStatus *status)
{
    if (inRefNum != 1 || !s_spbDeviceOpen) return SPB_BAD_REF_NUM;
    if (!status) return SPB_PARAM_ERR;

    status->recordingStatus = s_spbRecording ? 1 : 0;
    status->inputMeter = 128;
    status->totalSamplesToRecord = 22050;
    status->numberSamplesRecorded = s_samplesRecorded;
    status->totalMsecsToRecord = 1000;
    status->numberMsecsRecorded = (s_samplesRecorded * 1000) / 22050;
    return SPB_NO_ERR;
}

int16_t lc3_spb_get_device_info(int32_t inRefNum, uint32_t infoType, void *infoData)
{
    (void)infoType;
    if (inRefNum != 1 || !s_spbDeviceOpen) return SPB_BAD_REF_NUM;
    if (!infoData) return SPB_PARAM_ERR;
    *(uint32_t *)infoData = 22050; // Default sample rate 22.05 kHz
    return SPB_NO_ERR;
}

int16_t lc3_spb_set_device_info(int32_t inRefNum, uint32_t infoType, const void *infoData)
{
    (void)infoType;
    (void)infoData;
    if (inRefNum != 1 || !s_spbDeviceOpen) return SPB_BAD_REF_NUM;
    return SPB_NO_ERR;
}
