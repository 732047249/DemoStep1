#ifndef NATIVE_WEBRTC_AEC_e7d8dcf97e0ee7ef5fd599335da8c0d5
#define NATIVE_WEBRTC_AEC_e7d8dcf97e0ee7ef5fd599335da8c0d5

// ----------------------------------------------------------------------------------------------

#include <jni.h>
#include <string.h>
#include <unistd.h>

#include "logcat.h"
#include "IYCWebRtcAEC.h"
#include "IAcousticEchoCanceler.h"

// ----------------------------------------------------------------------------------------------

class NativeWebRtcAEC : public IAcousticEchoCanceler
{
public:
    NativeWebRtcAEC();
    virtual ~NativeWebRtcAEC();

    int aecmInit(int sampleRate);
    void aecmClose();

    int aecmBufferFarEnd(jshort* farEndData, jshort nrOfSamples);
    int aecmProcess(jshort* nearEndNoisyData, jshort* outData, jshort nrOfSamples, jshort msInSndCardBuf);

    void setDeviceLatency(int latencyInMS);
    int getDeviceLatency();
    //yangrui 0814
    int getXDCoherence();
    int getXECoherence();
    int getAECRatio();
    int mXDCoherence;
    int mXECoherence;
    int mAECRatio;

private:
    void *aecm;
 //   AecMetrics metric;

    float mInScratcher_fl[1024];
    float mOutScratcher_fl[1024];
};

#endif // NATIVE_WEBRTC_AEC_e7d8dcf97e0ee7ef5fd599335da8c0d5
