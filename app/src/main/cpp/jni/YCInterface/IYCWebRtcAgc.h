//
// Created by David on 16/12/19.
//

#ifndef ZAYHU_ANDROID_IYCWEBRTCAGC_H
#define ZAYHU_ANDROID_IYCWEBRTCAGC_H

#include "webrtc/modules/audio_processing/agc/legacy/gain_control.h"

int IYCWebRtcAgc_Create(void** agcInst);

int IYCWebRtcAgc_Init(void* agcInst,
                   int32_t minLevel,
                   int32_t maxLevel,
                   int16_t agcMode,
                   uint32_t fs);

int IYCWebRtcAgc_get_config(void* agcInst, WebRtcAgcConfig* config);

int IYCWebRtcAgc_set_config(void* agcInst, WebRtcAgcConfig agcConfig);

int IYCWebRtcAgc_AddFarend(void* state, const int16_t* in_far, size_t samples);

int IYCWebRtcAgc_Process(void* agcInst,
                      const int16_t* inNear,
                      const int16_t* inNear_H,
                      int16_t samples,
                      int16_t* out,
                      int16_t* out_H,
                      int32_t inMicLevel,
                      int32_t* outMicLevel,
                      int16_t echo,
                      uint8_t* saturationWarning);

int IYCWebRtcAgc_VirtualMic(void *agcInst, int16_t *in_near, int16_t *in_near_H,
                         int16_t samples, int32_t micLevelIn,
                         int32_t *micLevelOut);

void IYCWebRtcAgc_Free(void* state);

#endif //ZAYHU_ANDROID_IYCWEBRTCAGC_H
