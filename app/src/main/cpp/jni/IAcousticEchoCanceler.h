#ifndef IACOUSTIC_ECHO_CANCELER_321ac4164847f86f0e221bf76b7e0ab5
#define IACOUSTIC_ECHO_CANCELER_321ac4164847f86f0e221bf76b7e0ab5

class IAcousticEchoCanceler
{
public:
    virtual ~IAcousticEchoCanceler() {};

    virtual int aecmInit(int sampleRate) = 0;
    virtual void aecmClose() = 0;
    virtual int aecmBufferFarEnd(jshort* farEndData, jshort nrOfSamples) = 0;
    virtual int aecmProcess(jshort* nearEndNoisyData, jshort* outData, jshort nrOfSamples, jshort msInSndCardBuf) = 0;

    virtual void setDeviceLatency(int latencyInMS) = 0;
    virtual int getDeviceLatency() = 0;
    //yangrui 0814
    virtual int getXDCoherence() = 0;
    virtual int getXECoherence() = 0;
    virtual int getAECRatio() = 0;
};

#endif // IACOUSTIC_ECHO_CANCELER_321ac4164847f86f0e221bf76b7e0ab5
