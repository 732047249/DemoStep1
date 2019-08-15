//
// Created by David on 16/12/19.
//

#ifndef ZAYHU_ANDROID_IYCWEBRTCAEC_H
#define ZAYHU_ANDROID_IYCWEBRTCAEC_H

#include "webrtc/modules/audio_processing/aec/echo_cancellation.h"
#include "webrtc/modules/audio_processing/aec/aec_core.h"
using namespace webrtc;


int32_t IYCWebRtcAec_Create(void** aecInst);

int32_t IYCWebRtcAec_Init(void* aecInst, int32_t sampFreq, int32_t scSampFreq);

AecCore* IYCWebRtcAec_aec_core(void* handle);

int32_t IYCWebRtcAec_set_config(void* handle, webrtc::AecConfig config);

int IYCWebRtcAec_system_delay(webrtc::AecCore* aecCore);
//yangrui 0814
int IYCWebRtcAec_XDCoherence(webrtc::AecCore* aecCore);
int IYCWebRtcAec_XECoherence(webrtc::AecCore* aecCore);
int IYCWebRtcAec_AECRatio(webrtc::AecCore* aecCore);

void IYCWebRtcAec_SetSystemDelay(webrtc::AecCore* aecCore, int delay);

void IYCWebRtcAec_enable_extended_filter(webrtc::AecCore* aec, int enable);

int32_t IYCWebRtcAec_BufferFarend(void* aecInst,
                               const void* farend,
                               int16_t nrOfSamples);

int32_t IYCWebRtcAec_Process(void* aecInst,
                          const void* nearend,
                          const void* nearendH,
                          void* out,
                          void* outH,
                          int16_t nrOfSamples,
                          int16_t msInSndCardBuf,
                          int32_t skew, int32_t &xdCoherence, int32_t &xeCoherence, int32_t &aecRatio);

void IYCWebRtcAec_Free(void* aecInst);

#endif //ZAYHU_ANDROID_IYCWEBRTCAEC_H
