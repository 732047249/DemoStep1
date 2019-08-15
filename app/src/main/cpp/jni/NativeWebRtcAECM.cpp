 // ----------------------------------------------------------------------------------------------

#include "NativeWebRtcAECM.h"
#include"logcat.h"

// ----------------------------------------------------------------------------------------------

NativeWebRtcAECM::NativeWebRtcAECM()
        : aecm(NULL) {
}

NativeWebRtcAECM::~NativeWebRtcAECM() {
}

int NativeWebRtcAECM::aecmInit(int sampleRate) {
    LOGI("MobileAEC: AECM sample rate: %d, %08x", sampleRate, (unsigned int ) this);

    jint ret;

    ret = IYCWebRtcAecm_Create(&aecm);
    if (ret != 0) return ret;

    ret = IYCWebRtcAecm_Init(aecm, sampleRate);
    if (ret != 0) return ret;

    AecmConfig config;
    // int16_t cngMode;            // AECM_FALSE, AECM_TRUE (default)
    // int16_t echoMode;           // 0, 1, 2, 3 (default), 4

    config.cngMode = AecmTrue;
    config.echoMode = 4;

    //yangrui 0814
    mXDCoherence = 0;
    mXECoherence = 0;
    mAECRatio = 0;

    ret = IYCWebRtcAecm_set_config(aecm, config);
    if (ret != 0) return ret;

    return ret;
}

void NativeWebRtcAECM::aecmClose() {
    LOGI("MobileAEC: close %08x", (unsigned int ) this);


    IYCWebRtcAecm_Free(aecm);
    aecm = NULL;
}

int NativeWebRtcAECM::aecmBufferFarEnd(jshort* farEndData, jshort nrOfSamples) {
    int result = 0;
    for (int i = 0; i < nrOfSamples; i += 160) {
        result = IYCWebRtcAecm_BufferFarend(aecm, farEndData + i, 160);
    }

    return result;
}

int NativeWebRtcAECM::aecmProcess(jshort* nearEndNoisyData, jshort* outData, jshort nrOfSamples,
        jshort msInSndCardBuf) {
    jint ret = 0;

    //yangrui
   // LOGI("%s_%d",__FILE__,__LINE__);
    for (int i = 0; i < nrOfSamples; i += 160) {
    //yangrui 0814
        ret = IYCWebRtcAecm_Process(aecm, nearEndNoisyData + i, NULL, outData + i, 160, msInSndCardBuf,mXDCoherence,mXECoherence,mAECRatio);
        if (ret != 0) {
            return ret;
        }
    }

   // LOGI("%s_%d, XDCoherence : %d, XECoherence: %d",__FILE__,__LINE__ , mXDCoherence, mXECoherence);

    return ret;
}

void NativeWebRtcAECM::setDeviceLatency(int latencyInMS) {
}

int NativeWebRtcAECM::getDeviceLatency() {
    return -1;
}
//yangrui 0814
int NativeWebRtcAECM::getXDCoherence() {
//LOGI("%s_%d, XDCoherence : %d",__FILE__,__LINE__ , mXDCoherence);
    return  mXDCoherence;
}
int NativeWebRtcAECM::getXECoherence() {
//LOGI("%s_%d, XECoherence : %d",__FILE__,__LINE__ , mXECoherence);
    return  mXECoherence;
}
int NativeWebRtcAECM::getAECRatio() {
//LOGI("%s_%d, AECRatio : %d",__FILE__,__LINE__ , mAECRatio);
    return  mAECRatio;
}