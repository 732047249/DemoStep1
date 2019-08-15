#ifndef NATIVE_WEBRTC_AECM_A906AB175E6A4F33930FEB6B5632AF41
#define NATIVE_WEBRTC_AECM_A906AB175E6A4F33930FEB6B5632AF41

// ----------------------------------------------------------------------------------------------

#include <jni.h>
#include <string.h>
#include <unistd.h>

#include "logcat.h"
#include "IYCWebRtcAECM.h"
#include "IAcousticEchoCanceler.h"

// ----------------------------------------------------------------------------------------------

class NativeWebRtcAECM : public IAcousticEchoCanceler
{
public:
    NativeWebRtcAECM();
    virtual ~NativeWebRtcAECM();

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
};

#endif // NATIVE_WEBRTC_AECM_A906AB175E6A4F33930FEB6B5632AF41
