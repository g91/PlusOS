/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Input Manager (SPB / Microphone In)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SOUND_INPUT_H
#define LC3_SOUND_INPUT_H

#include <stdint.h>
#include <stdbool.h>

#define SPB_NO_ERR           0
#define SPB_DEVICE_BUSY    -227
#define SPB_BAD_REF_NUM    -228
#define SPB_PARAM_ERR       -50

typedef struct SPB {
    int32_t inRefNum;
    uint32_t count;
    uint32_t milliseconds;
    uint32_t bufferLength;
    void *bufferPtr;
    void (*completionRoutine)(struct SPB *inParamPtr);
    void (*interruptRoutine)(struct SPB *inParamPtr);
    int32_t userLong;
    int16_t error;
    int32_t unused;
} SPB, *SPBPtr;

typedef struct {
    int16_t recordingStatus;
    int16_t inputMeter;
    uint32_t totalSamplesToRecord;
    uint32_t numberSamplesRecorded;
    uint32_t totalMsecsToRecord;
    uint32_t numberMsecsRecorded;
} SPBStatus;

/* Sound Input Manager API */
int16_t lc3_spb_open_device(const char *deviceName, int16_t permission, int32_t *inRefNum);
int16_t lc3_spb_close_device(int32_t inRefNum);
int16_t lc3_spb_record(SPBPtr inParamPtr, bool asynchFlag);
int16_t lc3_spb_pause_recording(int32_t inRefNum);
int16_t lc3_spb_resume_recording(int32_t inRefNum);
int16_t lc3_spb_stop_recording(int32_t inRefNum);
int16_t lc3_spb_get_status(int32_t inRefNum, SPBStatus *status);
int16_t lc3_spb_get_device_info(int32_t inRefNum, uint32_t infoType, void *infoData);
int16_t lc3_spb_set_device_info(int32_t inRefNum, uint32_t infoType, const void *infoData);

#endif /* LC3_SOUND_INPUT_H */
