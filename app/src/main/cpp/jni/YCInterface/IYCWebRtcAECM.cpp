//
// Created by David on 16/12/19.
//

#include "IYCWebRtcAECM.h"
#include "logcat.h"

int32_t IYCWebRtcAecm_Create(void** aecInst) {
    void *self = WebRtcAecm_Create();
    *aecInst = self;

    if (self != NULL) {
        return 0;
    } else {
        return -1;
    }

}

int32_t IYCWebRtcAecm_Init(void *aecmInst, int32_t sampFreq) {
    int ret = WebRtcAecm_Init(aecmInst, sampFreq);
    return ret;
}

int32_t IYCWebRtcAecm_set_config(void *aecmInst, AecmConfig config) {
    int ret = WebRtcAecm_set_config(aecmInst, config);
    return ret;
}

void IYCWebRtcAecm_Free(void* aecmInst) {
    WebRtcAecm_Free(aecmInst);
}

int32_t IYCWebRtcAecm_BufferFarend(void *aecmInst, const void *farend,
                                size_t nrOfSamples) {
    int ret = WebRtcAecm_BufferFarend(aecmInst, (const int16_t *)farend, nrOfSamples);
    return ret;
}

int32_t IYCWebRtcAecm_Process(void *aecmInst, const void *nearendNoisy,
                           const void *nearendClean, void *out,
                           size_t nrOfSamples, int16_t msInSndCardBuf,  int32_t &xdCoherence, int32_t &xeCoherence, int32_t &aecRatio)
{
   //yangrui 0814
    int ret = WebRtcAecm_Process(aecmInst, (const int16_t *)nearendNoisy, (const int16_t *)nearendClean, (int16_t *)out, nrOfSamples, msInSndCardBuf,xdCoherence, xeCoherence, aecRatio);
 //   LOGI("%s_%d, XDCoherence : %d",__FILE__,__LINE__ , xdCoherence);
    return ret;
}

