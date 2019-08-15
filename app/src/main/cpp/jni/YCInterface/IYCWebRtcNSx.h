//
// Created by David on 16/12/19.
//

#ifndef ZAYHU_ANDROID_IYCWEBRTCNSX_H
#define ZAYHU_ANDROID_IYCWEBRTCNSX_H

#include <webrtc/modules/audio_processing/ns/noise_suppression_x.h>
#include <webrtc/modules/audio_processing/ns/nsx_core.h>

int IYCWebRtcNsx_Create(NsxHandle** nsxInst);

int IYCWebRtcNsx_Process(NsxHandle* nsxInst, short* speechFrame,
                           short* speechFrameHB, short* outFrame,
                           short* outFrameHB);

int IYCWebRtcNsx_Init(NsxHandle* nsxInst, uint32_t fs);

int IYCWebRtcNsx_set_policy(NsxHandle* nsxInst, int mode);

void IYCWebRtcNsx_Free(NsxHandle* nsxInst) ;

#endif //ZAYHU_ANDROID_IYCWEBRTCNSX_H
