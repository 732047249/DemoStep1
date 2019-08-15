//
// Created by David on 16/12/19.
//

#include "IYCWebRtcAEC.h"
#include "logcat.h"

int32_t IYCWebRtcAec_Create(void** aecInst) {
    void *self = WebRtcAec_Create();
    *aecInst = self;

    if (self != NULL) {
        return 0;
    } else {
        return -1;
    }

}

int32_t IYCWebRtcAec_Init(void* aecInst, int32_t sampFreq, int32_t scSampFreq) {
    int ret = WebRtcAec_Init(aecInst, sampFreq, scSampFreq);

    return ret;
}

AecCore* IYCWebRtcAec_aec_core(void* handle) {
    AecCore* aecCore = WebRtcAec_aec_core(handle);
    return aecCore;
}

int32_t IYCWebRtcAec_set_config(void* handle, webrtc::AecConfig config) {
    int ret = WebRtcAec_set_config(handle, config);
    return ret;
}

void IYCWebRtcAec_enable_extended_filter(webrtc::AecCore* aecCore, int enable) {
    WebRtcAec_enable_extended_filter(aecCore, enable);
}

int IYCWebRtcAec_system_delay(webrtc::AecCore* aecCore) {
    return WebRtcAec_system_delay(aecCore);
}

//yangrui 0814
int IYCWebRtcAec_XDCoherence(webrtc::AecCore* aecCore){
  //  LOGI("%s_%d, XDCoherence=: %d",__FILE__,__LINE__, WebRtcAec_XDCoherence(aecCore));
   return WebRtcAec_XDCoherence(aecCore);
    // return (int) (aecCore->hNlXdMean*1000);
 }
int IYCWebRtcAec_XECoherence(webrtc::AecCore* aecCore){
    return WebRtcAec_XECoherence(aecCore);
   // return (int) (aecCore->hNlDeMean*1000);
}
int IYCWebRtcAec_AECRatio(webrtc::AecCore* aecCore){
    return WebRtcAec_AECRatio(aecCore);
   // return (int) (aecCore->hAecRatio*1000);
}

void IYCWebRtcAec_SetSystemDelay(webrtc::AecCore* aecCore, int delay) {
    WebRtcAec_SetSystemDelay(aecCore, delay);
}

int32_t IYCWebRtcAec_BufferFarend(void* aecInst,
                               const void* farend,
                               int16_t nrOfSamples)
{
    int result = WebRtcAec_BufferFarend(aecInst, (const float*)farend, nrOfSamples);
    return result;
}

int32_t IYCWebRtcAec_Process(void* aecInst,
                          const void* nearend,
                          const void* nearendH,
                          void* out,
                          void* outH,
                          int16_t nrOfSamples,
                          int16_t msInSndCardBuf,
                          int32_t skew,
                          int32_t &xdCoherence, int32_t &xeCoherence, int32_t &aecRatio)
/*
int32_t WebRtcAec_Process(void* aecInst,
                          const float* const* nearend,
                          size_t num_bands,
                          float* const* out,
                          size_t nrOfSamples,
                          int16_t msInSndCardBuf,
                          int32_t skew)
                          */
{
    const float *nearend_float = (const float*)nearend;
    float *out_float = (float*)out;

    int ret = WebRtcAec_Process(aecInst, &nearend_float, 1, &out_float, nrOfSamples, msInSndCardBuf, skew,xdCoherence, xeCoherence, aecRatio);
    //LOGI("%s_%d, XDCoherence : %d",__FILE__,__LINE__ , xdCoherence);
       //yangrui
      // int err = WebRtcAec_GetMetrics(aecInst, &metric);
         // ERLE
      //LOGI("%s_%d,ERLE.instant=: %d, ERLE.average=: %d",__FILE__,__LINE__,metric.erle.average,metric.erle.instant);

    return ret;
}

void IYCWebRtcAec_Free(void* aecInst) {
    WebRtcAec_Free(aecInst);
}
