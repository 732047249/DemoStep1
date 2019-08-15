// ----------------------------------------------------------------------------------------------

#include "NativeWebRtcAEC.h"
#include "logcat.h"

// ----------------------------------------------------------------------------------------------

NativeWebRtcAEC::NativeWebRtcAEC()
        : aecm(NULL) {
    memset(mInScratcher_fl, 0, sizeof(mInScratcher_fl));
    memset(mOutScratcher_fl, 0, sizeof(mOutScratcher_fl));
    //yangrui
   // memset(&metric, 0, sizeof(metric));
}

NativeWebRtcAEC::~NativeWebRtcAEC() {
}

int NativeWebRtcAEC::aecmInit(int sampleRate) {
    LOGI("FullAEC: AEC sample rate: %d, %08x", sampleRate, (unsigned int ) this);

    jint ret;

    ret = IYCWebRtcAec_Create(&aecm);
    if (ret != 0) return ret;

    ret = IYCWebRtcAec_Init(aecm, sampleRate, sampleRate);
    if (ret != 0) return ret;

    AecConfig config;
    //yangrui  kAecFalse-->kAecTrue  --> kAecFalse
    config.metricsMode = kAecFalse;
    config.nlpMode = kAecNlpAggressive;
    config.skewMode = kAecFalse;
    config.delay_logging = kAecFalse;

    IYCWebRtcAec_enable_extended_filter(IYCWebRtcAec_aec_core(aecm), 1);
    ret = IYCWebRtcAec_set_config(aecm, config);
    if (ret != 0) return ret;

    return ret;
}

void NativeWebRtcAEC::aecmClose() {
    LOGI("FullAEC: close %08x", (unsigned int ) this);

    IYCWebRtcAec_Free(aecm);
    aecm = NULL;
}

int NativeWebRtcAEC::aecmBufferFarEnd(jshort* farEndData, jshort nrOfSamples) {
    int result = 0;
    for (int i = 0; i < nrOfSamples; i += 160) {
        for(int j = 0; j < 160; j++) {
            mInScratcher_fl[j] = (float) farEndData[i + j];
        }
        result = IYCWebRtcAec_BufferFarend(aecm, mInScratcher_fl, 160);
    }

    return result;
}

int NativeWebRtcAEC::aecmProcess(jshort* nearEndNoisyData, jshort* outData, jshort nrOfSamples, jshort msInSndCardBuf) {
    jint ret = 0;

    for (int i = 0; i < nrOfSamples; i += 160) {
        for(int j = 0; j < 160; j++) {
            mInScratcher_fl[j] = (float) nearEndNoisyData[i + j];
        }

        //yangrui
        ret = IYCWebRtcAec_Process(aecm, mInScratcher_fl, NULL, mOutScratcher_fl, NULL, 160, msInSndCardBuf, 0,mXDCoherence,mXECoherence,mAECRatio);

              /*
                //yangrui
                //UpdateMetrics
                static int erle_sum = 0;
                static int nVaildFrame = 0;
                 const int err = WebRtcAec_GetMetrics(aecm, &metric);
               if (err == 0) {
                 // ERLE
                  //  LOGI("%s_%d,ERL.instant=: %d, ERL.average=: %d",__FILE__,__LINE__,metric.erl.average,metric.erl.instant);
                    LOGI("%s_%d,ERLE.instant=: %d, ERLE.average=: %d",__FILE__,__LINE__,metric.erle.average,metric.erle.instant);
                  //  LOGI("%s_%d,RERL.instant=: %d, RERL.average=: %d",__FILE__,__LINE__,metric.rerl.average,metric.rerl.instant);
                  //  LOGI("%s_%d,aNLP.instant=: %d, aNLP.average=: %d",__FILE__,__LINE__,metric.aNlp.average,metric.aNlp.instant);
                            nVaildFrame++;
                            erle_sum += metric.erle.instant;
                        LOGI("%s_%d,nVaildFrame=: %d, Vaild_average=: %.2f",__FILE__,__LINE__,nVaildFrame,(double)erle_sum/nVaildFrame);

                    }
                    */

        for(int j = 0; j < 160; j++) {
            outData[i + j] = (short) (mOutScratcher_fl[j] + 0.5);
        }
        if (ret != 0) {
            return ret;
        }
    }

    return ret;
}

void NativeWebRtcAEC::setDeviceLatency(int latencyInMS) {
    IYCWebRtcAec_SetSystemDelay((AecCore*) aecm, latencyInMS);
}

int NativeWebRtcAEC::getDeviceLatency() {
    return IYCWebRtcAec_system_delay((AecCore*) aecm);
}

//yangrui 0814
int NativeWebRtcAEC::getXDCoherence() {

//LOGI("%s_%d, XDCoherence : %d",__FILE__,__LINE__ , mXDCoherence);
    return  mXDCoherence;
}
int NativeWebRtcAEC::getXECoherence() {
//LOGI("%s_%d, XECoherence : %d",__FILE__,__LINE__ , mXECoherence);
    return  mXECoherence;
}
int NativeWebRtcAEC::getAECRatio() {
//LOGI("%s_%d, AECRatio : %d",__FILE__,__LINE__ , mAECRatio);
    return  mAECRatio;
}
