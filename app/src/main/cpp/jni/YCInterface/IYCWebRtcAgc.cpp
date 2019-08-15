//
// Created by David on 16/12/19.
//

#include "IYCWebRtcAgc.h"

int IYCWebRtcAgc_Create(void** agcInst) {
    void* self = WebRtcAgc_Create();
    *agcInst = self;

    if (self != NULL) {
        return 0;
    } else {
        return -1;
    }

}

int IYCWebRtcAgc_Init(void* agcInst,
                   int32_t minLevel,
                   int32_t maxLevel,
                   int16_t agcMode,
                   uint32_t fs) {
    int ret = WebRtcAgc_Init(agcInst, minLevel, maxLevel, agcMode, fs);
    return ret;
}

int IYCWebRtcAgc_get_config(void* agcInst, WebRtcAgcConfig* config) {
    int ret = WebRtcAgc_get_config(agcInst, config);
    return ret;
}

int IYCWebRtcAgc_set_config(void* agcInst, WebRtcAgcConfig agcConfig) {
    int ret = WebRtcAgc_set_config(agcInst, agcConfig);
    return ret;
}
int IYCWebRtcAgc_AddFarend(void* state, const int16_t* in_far, size_t samples) {
    int ret = WebRtcAgc_AddFarend(state, in_far, samples);
    return ret;
}

int IYCWebRtcAgc_Process(void* agcInst,
                      const int16_t* inNear,
                      const int16_t* inNear_H,
                      int16_t samples,
                      int16_t* out,
                      int16_t* out_H,
                      int32_t inMicLevel,
                      int32_t* outMicLevel,
                      int16_t echo,
                      uint8_t* saturationWarning)
{

    int ret = WebRtcAgc_Process(agcInst,
                                &inNear,
                                1,
                                samples,
                                &out,
                                inMicLevel,
                                outMicLevel,
                                echo,
                                saturationWarning);
    return ret;
}

int IYCWebRtcAgc_VirtualMic(void *agcInst, int16_t *in_near, int16_t *in_near_H,
                         int16_t samples, int32_t micLevelIn,
                         int32_t *micLevelOut)
{
    int ret = WebRtcAgc_VirtualMic(agcInst,
                                   &in_near,
                                   1,
                                   samples,
                                   micLevelIn,
                                   micLevelOut);
    return ret;
}

void IYCWebRtcAgc_Free(void* state) {
    WebRtcAgc_Free(state);
}