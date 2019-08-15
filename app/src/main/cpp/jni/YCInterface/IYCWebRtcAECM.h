//
// Created by David on 16/12/19.
//

#ifndef ZAYHU_ANDROID_IYCWEBRTCAECM_H
#define ZAYHU_ANDROID_IYCWEBRTCAECM_H

#include "webrtc/modules/audio_processing/aecm/echo_control_mobile.h"

//#include "webrtc/modules/audio_processing/ns/include/noise_suppression_x.h"


int32_t IYCWebRtcAecm_Create(void** aecInst);

int32_t IYCWebRtcAecm_Init(void *aecmInst, int32_t sampFreq);

int32_t IYCWebRtcAecm_set_config(void *aecmInst, AecmConfig config);

void IYCWebRtcAecm_Free(void* aecmInst);

int32_t IYCWebRtcAecm_BufferFarend(void *aecmInst, const void *farend,
                                size_t nrOfSamples);

int32_t IYCWebRtcAecm_Process(void *aecmInst, const void *nearendNoisy,
                           const void *nearendClean, void *out,
                           size_t nrOfSamples, int16_t msInSndCardBuf, int32_t &xdCoherence, int32_t &xeCoherence, int32_t &aecRatio);

#endif //ZAYHU_ANDROID_IYCWEBRTCAECM_H
