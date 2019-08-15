// ----------------------------------------------------------------------------------------------

#include "NativeVoiceCodec.h"

#include "NativeWebRtcAEC.h"
#include "NativeWebRtcAECM.h"
#include "HighPassFilter.h"

#include "logcat.h"

#define abs(x) (((x) > 0) ? (x) : -(x))

static const float AGC_RATIO_MIN_DEFAULTS = 0.5f;

static const float AGC_RATIO_MAX = 2.5f;
static float AGC_RATIO_MIN = AGC_RATIO_MIN_DEFAULTS;
static const float AGC_TARGET_PERCENT = 32768 * 0.7f;

// ----------------------------------------------------------------------------------------------
static int saveNear(jshort* pcm, int len){

    {
        FILE * fp = fopen("/sdcard/near.pcm","a+");
        if(fp){
            fwrite(pcm,sizeof(jshort),len,fp);
            fclose(fp);
        }
    }
    return 0;
}

static int saveFar(jshort* pcm, int len){

    {
        FILE * fp = fopen("/sdcard/far.pcm","a+");
        if(fp){
            fwrite(pcm,sizeof(jshort),len,fp);
            fclose(fp);
        }
    }
    return 0;
}

static int saveAec(jshort* pcm, int len){

    {
        FILE * fp = fopen("/sdcard/aec.pcm","a+");
        if(fp){
            fwrite(pcm,sizeof(jshort),len,fp);
            fclose(fp);
        }
    }
    return 0;
}

NativeVoiceCodec::NativeVoiceCodec()
    :
    // flags
    mOpenRef(0), mCodecId(0), mSampleRate(0), mOpenFlags(0),

    // value states
    mStateFrameSize(0), mStateQuality(0), mStateDigitalAGC(0), mStateExpectedLoss(0), mStateDCOffset(0),

    // level states
    mStateAEC(0), mStateDecoderGain(0), mStateEncoderGain(0), mStateDecoderLPF(0), mStateEncoderLPF(0), mStateDecoderNR(
       0), mStateEncoderNR(0), mVoiceCount(0), mStateNSFrameSize(0), mEncoderHPFLevel(0) {
    LOGI("Yeecall AudioCodec created");

    // rnnoise
    mDenoiseState = rnnoise_create();
    // ----------------------------------------------------------------------------------------------
    // decoder related
    mDecoder = NULL;
    mNoiseSuppressionDecode = NULL;

    mDecoderLPFPrevSample = 0;

    mEncoder = NULL;
    mNoiseSuppressionEncode = NULL;
    mAGC = NULL;

    mEchoCanceler = NULL;
    mEchoCancelerPendingDelete =  NULL;

    mEncoderLPFPrevSample = 0;

    mEncodedFrameCount = 0;
    mDecodedFrameCount = 0;

    mOutsiderCount = 0;
    mInsiderCount = 0;
    mOutsiderLimit = 0;
    mInsiderLimit = 0;
    mNormalizeFactor = 0;

    memset(mSmoothPlayHistory, 0, sizeof(mSmoothPlayHistory));
    mSmoothPlayPosition = 0; // reset position

    memset(mSmoothRecordHistory, 0, sizeof(mSmoothPlayHistory));
    mSmoothRecordPosition = 0;

    memset(mNSBuffer, 0, sizeof(mNSBuffer));

    mWebRtcEncodeFilter = NULL;
    mWebRtcDecodeFilter = NULL;
    mUseDspFilter = false;

    mEncoderGainfactor = 1.0f;

    mEncoderHPF = NULL;

    isUsingHowlingDetect = 0;
    mHowling = NULL;
    fdStatus = 0;
    isMute = 0;

    mUsingNoiseDetect = -1;
    mLastNoiseDetectRet = -1;
    nFrameCount = 0;
    fNoiseReduceTotal = 0.0f;
    fNoiseReduceAver = 0.0f;
    fNoiseRatioTotal = 0.0f;
    fNoiseRatioAver = 0.0f;

    mClazzVoiceCodec = NULL;
    mMethodNoiseDetectID = NULL;
    mMethodNoiseReportID = NULL;

    mDenoiseMode = DENOISE_MODE_WEBRTC;
}

NativeVoiceCodec::~NativeVoiceCodec() {
    rnnoise_destroy(mDenoiseState);
}

// ----------------------------------------------------------------------------------------------

int NativeVoiceCodec::open(int codec, int sampleRate, int flags) {
    if (mOpenRef) {
        LOGE("error, codec already opened");
        return -1;
    }

    //LOGI("open: codec=%d, sampleRate=%d, flags=%d", codec, sampleRate, flags);

    // check sample rate
    if (sampleRate != 16000 && sampleRate != 8000) {
        return -1;
    }

    int ret = 0;
    int tmp = 0;

    // save
    mCodecId = codec;
    mSampleRate = sampleRate;
    mOpenFlags = flags;

    mStateQuality = 3;

    mEncodedFrameCount = 0;
    mDecodedFrameCount = 0;

    int result = 0;

    // ----------------------------------------------------------------------------------------------
    // setup opus codec

    mEncoder = opus_encoder_create(sampleRate, 1, OPUS_APPLICATION_VOIP, &result);
    if (result != OPUS_OK || !mEncoder) {
        LOGE("failed to create an opus encoder: %s\n", opus_strerror(result));
        return -1;
    }

    mDecoder = opus_decoder_create(sampleRate, 1, &result);
    if (result != OPUS_OK || !mDecoder) {
        LOGE("failed to create an opus decoder: %s\n", opus_strerror(result));
        return -1;
    }

    mStateFrameSize = mSampleRate * 0.02; // 20ms frame length
    mStateNSFrameSize = mSampleRate * 0.01; // 10ms frame length
    mStateAgcFrameSize = (mSampleRate * 0.01 > 160) ? 160 : mSampleRate * 0.01; // 10ms frame length, 160 at most

    // enable features: set bitrate, eanble vbr
    opus_encoder_ctl(mEncoder, OPUS_SET_VBR(1)); // default, not necessary
    opus_encoder_ctl(mEncoder, OPUS_SET_DTX(1));
    opus_encoder_ctl(mEncoder, OPUS_SET_INBAND_FEC(1));
    opus_encoder_ctl(mEncoder, OPUS_SET_COMPLEXITY(3)); // CPU on Nexus One: 0: 33%; 4: 40%; 6: 45%; 10: 50%
    opus_encoder_ctl(mEncoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(mEncoder, OPUS_SET_PACKET_LOSS_PERC(15));
    opus_encoder_ctl(mEncoder, OPUS_SET_PREDICTION_DISABLED(0)); // default, not necessary
    opus_encoder_ctl(mEncoder, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_MEDIUMBAND));
    opus_encoder_ctl(mEncoder, OPUS_SET_LSB_DEPTH(16)); // 16bit sampling depth
    opus_encoder_ctl(mEncoder, OPUS_SET_FORCE_CHANNELS(1));

    // finally, setup compression
    setCodecState(TYPE_QUALITY, mStateQuality);

    // ----------------------------------------------------------------------------------------------
    // setup NS

    ret = IYCWebRtcNsx_Create(&mNoiseSuppressionEncode);
    if (ret != 0) {
        LOGE("ERROR: could not perform noise suppression");
        mNoiseSuppressionEncode = NULL;
        mNoiseSuppressionDecode = NULL;
        return ret;
    }

    ret = IYCWebRtcNsx_Create(&mNoiseSuppressionDecode);
    if (ret != 0) {
        LOGE("ERROR: could not init noise suppression");
        mNoiseSuppressionEncode = NULL;
        mNoiseSuppressionDecode = NULL;
        return ret;
    }

    IYCWebRtcNsx_Init(mNoiseSuppressionEncode, mSampleRate);
    IYCWebRtcNsx_set_policy(mNoiseSuppressionEncode, 2);

    IYCWebRtcNsx_Init(mNoiseSuppressionDecode, mSampleRate);
    IYCWebRtcNsx_set_policy(mNoiseSuppressionDecode, 0);

    ret = IYCWebRtcAgc_Create(&mAGC);
    if (ret != 0) {
        LOGE("ERROR: could not create agc");
        mAGC = NULL;
        return ret;
    }

    ret = IYCWebRtcAgc_Init(mAGC, 0, 255, kAgcModeAdaptiveDigital, mSampleRate);
    if (ret != 0) {
        LOGE("ERROR: could not init agc");
    }

    mStateMicLevel = 0;

    WebRtcAgcConfig config = { 0 };
    ret = IYCWebRtcAgc_get_config(mAGC, &config);
    if (ret == 0) {
        config.targetLevelDbfs = 1; // -1 dbFS
        IYCWebRtcAgc_set_config(mAGC, config);

        LOGI("AGC: set targetLevelDbfs: %d, compressionGaindB: %d", config.targetLevelDbfs, config.compressionGaindB);
    }

    // ----------------------------------------------------------------------------------------------
    // setup dsp filter
    // ----------------------------------------------------------------------------------------------
    mWebRtcEncodeFilter = new NativeWebRtcFilter();
    if (mWebRtcEncodeFilter == NULL || !mWebRtcEncodeFilter->init(FILTER_MAX_INPUT_SIZE)) {
        LOGE("Failed to create encode filter");
        return -1;
    }

    mWebRtcDecodeFilter = new NativeWebRtcFilter();
    if (mWebRtcDecodeFilter == NULL || !mWebRtcDecodeFilter->init(FILTER_MAX_INPUT_SIZE)) {
        LOGE("Failed to create decode filter");
        return -1;
    }

    // ----------------------------------------------------------------------------------------------
    // setup high poss filter for recording
    mEncoderHPF = new HighPassFilter(mSampleRate);

    // ----------------------------------------------------------------------------------------------
    // setup AEC / AECM on demand, not here

    // ----------------------------------------------------------------------------------------------

    // clear other state
    mVoiceCount = 0;
    memset(mVoiceStats, 0, sizeof(mVoiceStats));

    mOpenRef++;

    //yangrui  output pcm
   // fNear = fopen("/sdcard/yeecall/near.pcm","rb");
  //  fFar = fopen("/sdcard/yeecall/far.pcm","wb+");
  //  fOut = fopen("/sdcard/yeecall/out.pcm","rb");

 //   openfile();
    return 0;
}

int NativeVoiceCodec::close() {
    if (!mOpenRef) {
        LOGE("Error: codec not opened. - close");
        return 32768;
    }

    mOpenRef--;

    LOGI("codec closed");

    opus_encoder_destroy(mEncoder);
    opus_decoder_destroy(mDecoder);

    IYCWebRtcNsx_Free(mNoiseSuppressionEncode);
    IYCWebRtcNsx_Free(mNoiseSuppressionDecode);

    IYCWebRtcAgc_Free(mAGC);

    mEncoder = NULL;
    mDecoder = NULL;
    mAGC = NULL;

    if (mEchoCanceler) {
        mEchoCanceler->aecmClose();
        delete mEchoCanceler;
        mEchoCanceler = NULL;
    }

    if (mWebRtcEncodeFilter) {
        mWebRtcEncodeFilter->destroy();
        delete mWebRtcEncodeFilter;
        mWebRtcEncodeFilter = NULL;
    }

    if (mWebRtcDecodeFilter) {
        mWebRtcDecodeFilter->destroy();
        delete mWebRtcDecodeFilter;
        mWebRtcDecodeFilter = NULL;
    }

    if (mEncoderHPF) {
        delete mEncoderHPF;
        mEncoderHPF = NULL;
    }

    if (mHowling) {
        closeFeedbackDetect(mHowling);
        mHowling = NULL;
    }

    if (mMethodNoiseDetectID != NULL) {
        mMethodNoiseDetectID = NULL;
    }

    if (mClazzVoiceCodec != NULL) {
        mClazzVoiceCodec = NULL;
    }

    mMethodNoiseReportID = NULL;

    //yangrui  output pcm
  //  fclose(fNear);
 //   fclose(fFar);
  //  fclose(fOut);
//closefile();
    return 0;
}

// ----------------------------------------------------------------------------------------------

int NativeVoiceCodec::encode(JNIEnv * env, jshort* farEndData, jshort* nearEndData, int msInSndCardBuf, jbyte* encodedData,
                             int maxEncodeLen) {
    if (!mOpenRef) {
        LOGE("Error: codec not opened. - encode: %d", mOpenRef);
        return -1;
    }

    if (!nearEndData || !encodedData) {
        // something wrong happens
        return -1;
    }

    if (isUsingHowlingDetect) {
        doHowlingDetect(nearEndData, mStateFrameSize);
        if (isMute) {
            memset(nearEndData, 0, mStateFrameSize* sizeof(short));
        }
    }

    mEncodedFrameCount++;

//    if (mEncodedFrameCount % 100 == 1) {
//        // update dc offset every 100 frames, 2 seconds
//        getCodecState(TYPE_DC_OFF_SET);
//    }

    if (mEncoderHPF && mEncoderHPFLevel != LEVEL_OFF) {
        // STEP 0: do high pass filter
        mEncoderHPF->filter(nearEndData, mStateFrameSize);
    }

    int ret = 0;

    // LOGI("check agc: far=%08x, near=%08x, agc=%08x, decoder=%08x, encoder=%08x, nsDec=%08x, nsEnc=%08x", farEndData,
    //        nearEndData, mAGC, mDecoder, mEncoder, mNoiseSuppressionDecode, mNoiseSuppressionEncode);

    // prepare for agc
    int newMicLevel = mStateMicLevel;
    if (farEndData) {
        for (int i = 0; i < mStateFrameSize; i += mStateAgcFrameSize) {
            // accept 10ms frame only
            ret = IYCWebRtcAgc_AddFarend(mAGC, farEndData + i, mStateAgcFrameSize);
            if ( ret != 0) {
                LOGI("could not add farend for agc: %d, mStateFrameSize=%d", ret, mStateFrameSize);
                break;
            }
        }
    }

    if (nearEndData) {
        ret = IYCWebRtcAgc_VirtualMic(mAGC, nearEndData, NULL, mStateFrameSize, mStateMicLevel, &newMicLevel);
        // LOGI("micLevel: %d --> %d, ret=%d", mStateMicLevel, newMicLevel, ret);
        mStateMicLevel = newMicLevel;
    }

#if 0
    int audioSampleMax = 0;

    if (mStateDCOffset != 0 && false) {
        // STEP 1: dc offset correction & peak value searching
        for (int i = 0; i < mStateFrameSize; i++) {
            short valueShortRAW = nearEndData[i];
            short valueShort = valueShortRAW - mStateDCOffset;
            short valueShortABS = abs(valueShort);

            // perform history statistics, count with raw values
            int value = (valueShortRAW) & 0xffff;
            mVoiceStats[value]++;

            // perform dc offset correction
            nearEndData[i] = valueShort;

            // check for peak
            int valueAbs = abs(nearEndData[i]);
            if (valueAbs > audioSampleMax) {
                audioSampleMax = valueAbs;
            }
        }
    }

    // LOGI("peak: %d, offset: %d", audioSampleMax, mStateDCOffset);
#endif

    mVoiceCount += mStateFrameSize;

    // STEP 2: echo canceling

      //yangrui
    //   AecMetrics metrics;
     //   memset(&metrics, 0, sizeof(metrics));


    IAcousticEchoCanceler* echoCanceler = mEchoCanceler;
    if (mStateAEC != AEC_TYPE_DISABLE && echoCanceler) {
       //  LOGI("doing aec with: %d, msInSndCardBuf: %d, farEnd: 0x%08x, nearEnd: 0x%08x", mStateAEC, msInSndCardBuf, farEndData, nearEndData);
        int aecOk = 1;

        if (farEndData && nearEndData) {
            static short lastDelay = 0;
            if (abs(lastDelay - msInSndCardBuf) > 3) {
                LOGI("xxx delay: %d", msInSndCardBuf);
                lastDelay = msInSndCardBuf;
            }

            for (int i = 0; i < mStateFrameSize; i += 160) {
                echoCanceler->aecmBufferFarEnd(farEndData + i, 160);


                aecOk = echoCanceler->aecmProcess(nearEndData + i, mEncoderBufferRaw + i, 160, msInSndCardBuf);

               /*
                //yangrui  Output pcm data for analysis
                if(NULL != fFar)
                {
                    LOGI("%s_%d   Writting pcm data…………",__FILE__,__LINE__);
                    fwrite(farEndData + i, sizeof(jshort), 160, fFar);
                 }
                if(NULL != fNear)
                   fwrite(nearEndData + i, sizeof(jshort), 160, fNear);
                if(NULL != fOut)
                   fwrite(mEncoderBufferRaw + i, sizeof(jshort), 160, fOut);

                     WebRtcAec_GetMetrics(echoCanceler, &metrics);
                     // ERLE
                     LOGI("ERLE.instant=: %d, ERLE.average=: %d",metrics.erle.average,metrics.erle.instant);
                  */


            }
        }

        if (aecOk != 0) {
            // LOGI("farEnd=0x%08x, nearEnd=0x%08x", farEndData, nearEndData);
            memcpy(mEncoderBufferRaw, nearEndData, sizeof(short) * mStateFrameSize);
        }
    }
    else {
        // copy data back if no digital agc enabled, other wise data already stored in mEncoderBufferRaw
        memcpy(mEncoderBufferRaw, nearEndData, sizeof(short) * mStateFrameSize);
    }

    // drop the pending deleted echo canceler
    echoCanceler = mEchoCancelerPendingDelete;
    mEchoCancelerPendingDelete = NULL;
    if (echoCanceler) {
        delete echoCanceler;
    }
    echoCanceler = NULL;

/*
           if(nearEndData)
            {
             //   LOGI("%s_%d   Writting pcm data…………",__FILE__,__LINE__);
            //   saveNear(nearEndData,mStateFrameSize);

             }

             if(farEndData)
             {
                    //     LOGI("%s_%d   Writting pcm data…………",__FILE__,__LINE__);

                   //     saveFar(farEndData,mStateFrameSize);
                   //     saveAec(mEncoderBufferRaw,mStateFrameSize);
             }
*/

#if 0
    // STEP 3: Digital AGC & Extra encoder gain
    if (false && mStateDigitalAGC > 0) {
        // LOGI("doing digital agc %d ...", mStateDigitalAGC);

        short* srcData = (mStateAEC != AEC_TYPE_DISABLE && mEchoCanceler) ? mEncoderBufferRaw : nearEndData;

        int NOISE_GATE_LOW = 3;
        int NOISE_GATE_HIGH = 32000;

        for (int i = 0; i < mStateFrameSize; i++) {
            short valueShortRAW = srcData[i];
            short valueShortABS = abs(valueShortRAW);

            if (valueShortABS > NOISE_GATE_LOW && valueShortABS < NOISE_GATE_HIGH) {
                // ignore pure silent
                if (valueShortABS > mStateDigitalAGC) {
                    mOutsiderCount++;
                }
                else {
                    mInsiderCount++;
                }

                if (mNormalizeFactor > AGC_RATIO_MIN && mOutsiderCount > mOutsiderLimit) {
                    // range is too small, enlarge it & perform normalize
                    mOutsiderCount = 0;
                    mInsiderCount = 0;

                    mStateDigitalAGC += 400;// enlarge range to prevent clip

                    mNormalizeFactor = AGC_TARGET_PERCENT / mStateDigitalAGC;

                    if (mNormalizeFactor < AGC_RATIO_MIN) {
                        mNormalizeFactor = AGC_RATIO_MIN;
                    }
                    else if (mNormalizeFactor > AGC_RATIO_MAX) {
                        mNormalizeFactor = AGC_RATIO_MAX;
                    }

                    LOGE("AGC ++ : factor=%f, max=%d, offset=%d, encoderGain=%d", mNormalizeFactor, mStateDigitalAGC,
                         mStateDCOffset, mStateEncoderGain);
                }
                else if (mNormalizeFactor < AGC_RATIO_MAX && mInsiderCount > mInsiderLimit) { // at least 2 seconds of audio data
                    // range is too large, shrink it and enlarge volumn
                    mOutsiderCount = 0;
                    mInsiderCount = 0;

                    mStateDigitalAGC -= 100;

                    mNormalizeFactor = AGC_TARGET_PERCENT / mStateDigitalAGC;

                    if (mNormalizeFactor < AGC_RATIO_MIN) {
                        mNormalizeFactor = AGC_RATIO_MIN;
                    }
                    else if (mNormalizeFactor > AGC_RATIO_MAX) {
                        mNormalizeFactor = AGC_RATIO_MAX;
                    }

                    LOGE("AGC -- : factor=%f, max=%d, offset=%d, encoderGain=%d", mNormalizeFactor, mStateDigitalAGC,
                         mStateDCOffset, mStateEncoderGain);
                }
            }

            // normalize offset values: ignore values under noise gate
            if (valueShortABS) {
                switch (mStateEncoderGain) {
                case LEVEL_OFF:
                    mEncoderBufferRaw[i] = valueShortRAW * mNormalizeFactor;
                    break;
                case LEVEL_MODERATE:
                    mEncoderBufferRaw[i] = valueShortRAW * mNormalizeFactor * 1.25f;
                    break;
                case LEVEL_STANDARD:
                    mEncoderBufferRaw[i] = valueShortRAW * mNormalizeFactor * 1.5f;
                    break;
                case LEVEL_ENHANCED:
                    mEncoderBufferRaw[i] = valueShortRAW * mNormalizeFactor * 2.0f;
                    break;
                }
            }
            else {
                mEncoderBufferRaw[i] = 0;
            }

            // LOGI("%d - factor: %f, gain: %d", mEncoderBufferRaw[i], mNormalizeFactor, mStateEncoderGain);
        }
    }
#endif

    // apply extra encoder gain if needed
    if (mStateEncoderGain != LEVEL_OFF) {
        // LOGI("factor: %f", mEncoderGainfactor);

        if (abs(mEncoderGainfactor - 1) > 0.01) {
            for (int i = 0; i < mStateFrameSize; i++) {
                short valueShortRAW = mEncoderBufferRaw[i];
                if (valueShortRAW) {
                    mEncoderBufferRaw[i] = valueShortRAW * mEncoderGainfactor;
                }
            }
        }
    }

    // LOGI("encoder gain: %d, nr: %d", mStateEncoderGain, mStateEncoderNR);

    // now processed data store in mEncoderBufferRaw

    // STEP 4: Noise Reduction
    bool nsOk = true;

    //select different noise suppression methods
    if (mDenoiseMode == DENOISE_MODE_WEBRTC) {
        if (mStateEncoderNR && mNoiseSuppressionEncode) {
            // noise suppression: 80 samples per action
             for (int i = 0; i < mStateFrameSize; i += mStateNSFrameSize) {
                // accept 10ms frame only
                 int ret = IYCWebRtcNsx_Process(mNoiseSuppressionEncode, mEncoderBufferRaw + i, NULL, nearEndData + i, NULL);
                 if (ret < 0) {
                //    LOGI("ns ret=%d", ret);
                    nsOk = false;
                   break;
                 }
             }
             if (!nsOk) {
                 // copy data if not processed
                 memcpy(nearEndData, mEncoderBufferRaw, sizeof(short) * mStateFrameSize);
              }

        } else {
              // copy data if not processed
              memcpy(nearEndData, mEncoderBufferRaw, sizeof(short) * mStateFrameSize);
        }
    } else if (mDenoiseMode == DENOISE_MODE_INTELLIGENCE) {
        if (mDenoiseState) {
            // copy 320 samples to mNSBuffer
            for (int i = 0; i < mStateFrameSize; i++) {
                mNSBuffer[i ] = mEncoderBufferRaw[i];
            }
            for (int i = 0; i < FRAME_SIZE; i++) {
                mFrameData[i] = mNSBuffer[i];
            }
            // rnn noise suppression: 320 samples per action
            int ret = rnnoise_process_frame(mDenoiseState, mFrameData, mFrameData);
            if (ret < 0) {
               //  LOGI("ns ret=%d", ret);
               nsOk = false;
               // copy data if not processed correctly
               memcpy(nearEndData, mEncoderBufferRaw, sizeof(short) * mStateFrameSize);
            } else {
                //copy 320 samples from mFrameData
                for (int i = 0; i < mStateFrameSize; i++) {
                   nearEndData[i] = mFrameData[i];
                }
            }
        } else {
            // copy data if not processed
            memcpy(nearEndData, mEncoderBufferRaw, sizeof(short) * mStateFrameSize);
        }
    } else { // mDenoiseMode == DENOISE_MODE_NONE or Unknown mode
      // copy data if not processed
      memcpy(nearEndData, mEncoderBufferRaw, sizeof(short) * mStateFrameSize);
    }

   //    if(NULL != fOut)
  //        fwrite(nearEndData, sizeof(jshort), mStateFrameSize, fOut);

   fNoiseRatioBefore = getNoiseRatio(mEncoderBufferRaw, mStateFrameSize);
   fNoiseRatioAfter = getNoiseRatio(nearEndData, mStateFrameSize);

    fNoiseReduceTotal += (fNoiseRatioBefore - fNoiseRatioAfter);

    fNoiseRatioTotal += fNoiseRatioAfter;

    //Metric for Noise Suppression
    // if (mEncodedFrameCount > 0) {
    //   fNoiseReduceAver = fNoiseReduceTotal / mEncodedFrameCount;
    //   fNoiseRatioAver = fNoiseRatioTotal / mEncodedFrameCount;
    // }
    if ((mEncodedFrameCount % 200) == 199) {
       fNoiseReduceAver = fNoiseReduceTotal / 200;
       fNoiseRatioAver = fNoiseRatioTotal / 200;
       fNoiseReduceTotal= 0;
       fNoiseRatioTotal = 0;
       // if (mClazzVoiceCodec == NULL) { FIXME
       //   mClazzVoiceCodec = env->FindClass("com/zayhu/library/jni/VoiceCodec");
       //   if (mClazzVoiceCodec == NULL) {
       //      LOGI("Noise report can't find callback clazz");
       //   }
       // }
       // if (mClazzVoiceCodec != NULL && mMethodNoiseReportID == NULL) { //FIXME
       // if (mClazzVoiceCodec != NULL) {
       //    mMethodNoiseReportID = env->GetStaticMethodID(mClazzVoiceCodec, "noiseReport", "(II)V");
       //    if (mMethodNoiseReportID == NULL) {
       //      LOGI("Noise report can't find callback method");
       //  }
       // }
       // if (mClazzVoiceCodec != NULL && mMethodNoiseReportID != NULL) {
       //   env->CallStaticVoidMethod(mClazzVoiceCodec, mMethodNoiseReportID, (int)(fNoiseReduceAver * 10000), (int)(fNoiseRatioAver * 10000));
       // }
       if (env != NULL) {
          jclass clazzVoiceCodec = env->FindClass("com/zayhu/library/jni/VoiceCodec");
          if (clazzVoiceCodec != NULL) {
             jmethodID methodNoiseReport = env->GetStaticMethodID(clazzVoiceCodec, "noiseReport", "(II)V");
             if (methodNoiseReport != NULL) {
                env->CallStaticVoidMethod(clazzVoiceCodec, methodNoiseReport, (int)(fNoiseReduceAver * 10000), (int)(fNoiseRatioAver * 10000));
             }
             methodNoiseReport = NULL;
          }
          if (clazzVoiceCodec) {
             env->DeleteLocalRef(clazzVoiceCodec);
          }
          clazzVoiceCodec = NULL;
       }
    }
 //   LOGI("Noise Reduce: fNoiseReduce = %f, fNoiseRatioAfter = %f, fNoiseRatioAver = %f, fNoiseReduceAver = %f",
 //   (fNoiseRatioBefore - fNoiseRatioAfter) * mStateFrameSize, fNoiseRatioAfter, fNoiseRatioAver, fNoiseReduceAver);

    // Step: Strong Noise Detection

    // if (false && mUsingNoiseDetect) {
    //     int ret = doNoiseDetect(nearEndData, mStateFrameSize, nFrameCount);
         // if(nFrameCount %FRAME_NUM == FRAME_NUM-1)
         //    LOGI("Noise detect: nFrameCount = %d, ret = %d", nFrameCount, ret);
    //     nFrameCount++;

    //     if (ret != 0 && ret != mLastNoiseDetectRet) {
            // detect result changed, notify
    //        if (mClazzVoiceCodec == NULL) {
    //           mClazzVoiceCodec = env->FindClass("com/zayhu/library/jni/VoiceCodec");
    //           if (mClazzVoiceCodec == NULL) {
    //              LOGI("Noise detect can't find callback clazz");
    //           }
    //        }
    //        if (mMethodNoiseDetectID == NULL &&  mClazzVoiceCodec != NULL) {
    //           mMethodNoiseDetectID = env->GetStaticMethodID(mClazzVoiceCodec, "notifyNoiseDetect", "(I)V");
    //           if (mMethodNoiseDetectID == NULL) {
    //              LOGI("Noise detect can't find callback method");
    //           }
    //        }
    //        if (mMethodNoiseDetectID != NULL && mClazzVoiceCodec != NULL) {
    //           env->CallStaticVoidMethod(mClazzVoiceCodec, mMethodNoiseDetectID, ret);
    //        }
    //        mLastNoiseDetectRet = ret;
    //     }
    // }



    // STEP 5: LPF for recording
    if (mUseDspFilter) {
        // Use DSP Filter for filter invalid frequency
        switch (mStateEncoderLPF) {
        case LEVEL_MODERATE:
        // TODO: use different cut off frequency
        case LEVEL_STANDARD:
        case LEVEL_ENHANCED: {
            bool filter_result = mWebRtcEncodeFilter->filter(nearEndData, mStateFrameSize, mWebRtcFilterEncodeResult);
            if (!filter_result) {
                LOGE("Failed to filter");
            }
            else {
                memcpy(nearEndData, mWebRtcFilterEncodeResult, sizeof(short) * mStateFrameSize);
            }
        }
        break;
        default:
            break;
        }
    }
    else {
        if (mStateEncoderLPF != LEVEL_OFF) {
            // LOGI("doing encoder LPF: %d ...", mStateEncoderLPF);

            switch (mStateEncoderLPF) {
            case LEVEL_MODERATE:
            //            for (int i = 0; i < mStateFrameSize; i++) {
            //                int old = nearEndData[i];
            //                nearEndData[i] = mEncoderLPFPrevSample * 0.1f + old * 0.9f + 0.5f;
            //                mEncoderLPFPrevSample = old;
            //            }
            //            break;
            case LEVEL_STANDARD:
            //            for (int i = 0; i < mStateFrameSize; i++) {
            //                int old = nearEndData[i];
            //                nearEndData[i] = mEncoderLPFPrevSample * 0.25f + old * 0.75f + 0.5f;
            //                mEncoderLPFPrevSample = old;
            //            }
            //            break;
            case LEVEL_ENHANCED:
                for (int i = 0; i < mStateFrameSize; i++) {
                    nearEndData[i] = smoothVoiceRecord(nearEndData[i]);
                }
                break;
            }
        }
    }

    // last step before encoding: AGC finalize
    uint8_t saturationWarning = 0;
    int prev = nearEndData[43];

    for (int i = 0; i < mStateFrameSize; i += mStateAgcFrameSize) {
        // accept 10ms frame only
        ret = IYCWebRtcAgc_Process(mAGC, nearEndData+i, NULL, mStateAgcFrameSize, nearEndData+i, NULL, mStateMicLevel, &newMicLevel,
                                   0, &saturationWarning);
    }

    if (saturationWarning) {
        int current = nearEndData[43];
        float percent = prev == 0 ? -200.0f : ((float) current) / prev;
        LOGI("saturationWarning: micLevel: %d --> %d, ret=%d, saturation: %d; AGC: %d --> %d, %4f", mStateMicLevel,
             newMicLevel, ret, saturationWarning, prev, current, percent);
    }

    mStateMicLevel = newMicLevel;

    // STEP 6: Encode to opus: accept multiple of 2.5ms frame
    int encodedBytes = opus_encode(mEncoder, nearEndData, mStateFrameSize, (unsigned char*) encodedData, 4096);
    // LOGI("Encoding: %d frames --> total: %d bytes", nrOfSamples, totalBytes);

    return (jint) encodedBytes;
}

int NativeVoiceCodec::decode(jbyte* encodedData, int encodedLen, jshort* decodedData, int maxDecodeLen, int decodeFEC) {
    if (!mOpenRef) {
        LOGE("Error: codec not opened. - decode: %d", mOpenRef);
        return -1;
    }

    mDecodedFrameCount++;

    // decode to internal buffer mDecoderBufferOutput
    int decodedLen = 0;

    bool lost = encodedData == NULL || encodedLen <= 0;
    if (lost) {
        // current frame is lost, could not do FEC, use PLC to recover this
        decodedLen = opus_decode(mDecoder, NULL, 0, (opus_int16 *) mDecoderBufferOutput, mStateFrameSize, 1);
    }
    else {
        decodedLen = opus_decode(mDecoder, (unsigned char*) encodedData, encodedLen,
                                 (opus_int16 *) mDecoderBufferOutput, mStateFrameSize, decodeFEC);
    }

    if (decodedLen > 0) {
        bool nsOk = false;

        // do additional pass of NR
        if (mStateDecoderNR != LEVEL_OFF && mNoiseSuppressionDecode) {
            // LOGI("doing decoder noise suppression ... %d", mStateDecoderNR);
            nsOk = true;

            // noise suppression: 80 samples per action
            for (int i = 0; i < mStateFrameSize; i += mStateNSFrameSize) {
                // accept 10ms frame only
                int ret = IYCWebRtcNsx_Process(mNoiseSuppressionDecode, mDecoderBufferOutput + i, NULL, decodedData + i,
                                               NULL);
                if (ret < 0) {
                    nsOk = false;
                    break;
                }
            }
        }

        if (!nsOk) {
            // copy to destination
            memcpy(decodedData, mDecoderBufferOutput, sizeof(short) * decodedLen);
        }

        if (mUseDspFilter) {
            switch (mStateDecoderLPF) {
            case LEVEL_MODERATE:
            case LEVEL_STANDARD:
            case LEVEL_ENHANCED: {
                bool filter_result = mWebRtcDecodeFilter->filter(decodedData, decodedLen, mWebRtcFilterDecodeResult);
                if (!filter_result) {
                    LOGE("Failed to filter");
                }
                else {
                    memcpy(decodedData, mWebRtcFilterDecodeResult, sizeof(short) * decodedLen);
                }
            }
            default:
                break;
            }
        }
        else {
            if (mStateDecoderLPF != LEVEL_OFF) {
                // LOGI("doing decoder LPF: %d", mStateDecoderLPF);

                switch (mStateDecoderLPF) {
                case LEVEL_MODERATE:
                //                for (int i = 0; i < decodedLen; i++) {
                //                    int old = decodedData[i];
                //                    decodedData[i] = mEncoderLPFPrevSample * 0.1f + old * 0.9f + 0.5f;
                //                    mDecoderLPFPrevSample = old;
                //                }
                //                break;
                case LEVEL_STANDARD:
                //                for (int i = 0; i < decodedLen; i++) {
                //                    int old = decodedData[i];
                //                    decodedData[i] = mEncoderLPFPrevSample * 0.25f + old * 0.75f + 0.5f;
                //                    mEncoderLPFPrevSample = old;
                //                }
                //                break;
                case LEVEL_ENHANCED:
                    for (int i = 0; i < decodedLen; i++) {
                        decodedData[i] = smoothVoicePlay(decodedData[i]);
                    }
                    break;
                }
            }
        }

        // LOGD("decoded: length=%d, nrOfSamples=%d", decodedLen, nrOfSamples);
    }
    else {
        LOGE("Error: decode error: %d", decodedLen);
        decodedLen = 0;
    }

    return (jint) decodedLen;
}

int NativeVoiceCodec::setCodecState(jint typeID, jint value) {
    // LOGI("set codec state: open=%d, type=%d (%08x), value=%d", mOpenRef, typeID, typeID, value);

    if (mOpenRef <= 0) {
        return -1;
    }

    if ((typeID & 0xf0) == 0x20) {
        if (value < LEVEL_MINUS_ENHANCED || value > LEVEL_ENHANCED) {
            LOGE("wrong codec state, ignored: type=%d (%08x), value=%d, check=%d", typeID, typeID, value,
                 (typeID & 0xf0));
            return -1;
        }
    }

    switch (typeID) {
    // value types
    case TYPE_FRAME_SIZE: {
        // not allowed
        return -1;
    }
    case TYPE_QUALITY: {
        mStateQuality = value;

        if (value <= 0) {
            value = 0;
        }
        else if (value >= 10) {
            value = 10;
        }

        int old = mStateQuality;

        mStateQuality = value;

        // Suggested bit rate from codec: 3000 + SampleRate (8KHz: 11kbps; 16KHz: 19kbps)
        // q=0:  (10kbps / 20kbps) * 2channel
        // q=10: (20kbps / 40kbps) * 2channel
        int bitRate = 0;

        if (mSampleRate > 10000) {
            bitRate = (10000 + 1000 * mStateQuality) * 2;
        } else {
            // https://en.wikipedia.org/wiki/Enhanced_Data_Rates_for_GSM_Evolution
            // https://en.wikipedia.org/wiki/General_Packet_Radio_Service
            //
            // 8K sample rate is used for 2g network only.
            // upload bandwidth should be 42.8kbps for GPRS. we can use 1/4 of this in average
            //
            bitRate = 6000 + 1000 * mStateQuality; // (6kbps ~ 16kbps) * 2 for in&out
        }

        // add more bit rate for packet loss
        bitRate += (mStateExpectedLoss / 10 + 1) * 1000;

        opus_encoder_ctl(mEncoder, OPUS_SET_BITRATE(bitRate));

        LOGI("set quality: compress=%d sampleRate=%d, loss=%d, bps=%d bits/s", mStateQuality, mSampleRate,
             mStateExpectedLoss, bitRate);

        return 0;
    }
    case TYPE_DIGITAL_AGC: {
        if (abs(value) > 32768) {
            return -1;
        }

        int old = mStateDigitalAGC;
        mStateDigitalAGC = value;

        bool shouldNormalize = value > 0;
        if (shouldNormalize) {
            // LOGI("enable digital agc: %d", mStateDigitalAGC);

            mStateDigitalAGC = value;  // initial range

            mOutsiderCount = 0;
            mInsiderCount = 0;

            mOutsiderLimit = (mStateEncoderGain != LEVEL_OFF) ? 30 : 10; // drop gain fast, but increase slower
            mInsiderLimit = mSampleRate; // 8KHz = 8000, 16KHz = 16000

            mNormalizeFactor = AGC_TARGET_PERCENT / mStateDigitalAGC;

            LOGI("digital agc: range=%d normalize factor: %f", mStateDigitalAGC, mNormalizeFactor);
        }
        else {
            LOGI("do not enable digital agc");
            mStateDigitalAGC = -1;
        }

        return 0;
    }
    case TYPE_EXPECTED_LOSS: {
        if (value < 5) {
            value = 5;
        }

        if (value > 50) {
            value = 50;
        }

        mStateExpectedLoss = value;
        opus_encoder_ctl(mEncoder, OPUS_SET_PACKET_LOSS_PERC(mStateExpectedLoss));

        // LOGI("set expected loss: %d", mStateExpectedLoss);

        return 0;
    }
    case TYPE_DC_OFF_SET: {
        mStateDCOffset = value;
        // LOGI("set dc offset: %d", mStateDCOffset);
        return 0;
    }
    // level types
    case TYPE_AEC: {
        LOGI("update aec mode: %d", value);

        switch (value) {
        case AEC_TYPE_DISABLE: {
            mStateAEC = value;
            if (mEchoCanceler) {
                mEchoCanceler->aecmClose();
                delete mEchoCanceler;
                mEchoCanceler = NULL;
            }
            return 0;
        }
        case AEC_TYPE_FULL: {
            mStateAEC = value;

            IAcousticEchoCanceler* oldAEC = mEchoCanceler;
            IAcousticEchoCanceler* newAEC = new NativeWebRtcAEC();
            if (newAEC->aecmInit(mSampleRate) != 0) {
                LOGE("AECM init failed");
            }

            mEchoCanceler = newAEC;
            mEchoCancelerPendingDelete = oldAEC; // do not delete immediately, it might in use

            return 0;
        }
        case AEC_TYPE_MOBILE: {
            mStateAEC = value;

            IAcousticEchoCanceler* oldAEC = mEchoCanceler;
            IAcousticEchoCanceler* newAEC = new NativeWebRtcAECM();
            if (newAEC->aecmInit(mSampleRate) != 0) {
                LOGE("AECM init failed");
            }

            mEchoCanceler = newAEC;
            mEchoCancelerPendingDelete = oldAEC; // do not delete immediately, it might in use

            return 0;
        }
        case AEC_TYPE_RESET: {
            int currentType = mStateAEC;
            IAcousticEchoCanceler* oldAEC = mEchoCanceler;
            IAcousticEchoCanceler* newAEC = NULL;

            switch (currentType) {
            case AEC_TYPE_DISABLE: {
                newAEC = NULL;
                break;
            }
            case AEC_TYPE_FULL: {
                newAEC = new NativeWebRtcAEC();
                if (newAEC->aecmInit(mSampleRate) != 0) {
                    LOGE("AEC init failed");
                }
                break;
            }
            case AEC_TYPE_MOBILE: {
                newAEC = new NativeWebRtcAECM();
                if (newAEC->aecmInit(mSampleRate) != 0) {
                    LOGE("AECM init failed");
                }
                break;
            }
            }

            mEchoCanceler = newAEC;
            mEchoCancelerPendingDelete = oldAEC; // do not delete immediately, it might in use

            return 0;
        }
        }

        return -1;
    }

    case TYPE_HOWLING: {
        isUsingHowlingDetect = value;
        if (value>0)
        {
            struct feedbackDetect *localmHowling = mHowling;
            if (mHowling) {
                mHowling = NULL;
                closeFeedbackDetect(localmHowling);
            }
            localmHowling = nativeHowlingDetectCreat();
            if (!localmHowling) {
                isUsingHowlingDetect = 0;
            } else {
                initialFeedbackDetect(localmHowling, mSampleRate);
            }
            mHowling = localmHowling;
        } else {
            if (mHowling) {
                struct feedbackDetect *localmHowling = mHowling;
                mHowling = NULL;
                closeFeedbackDetect(localmHowling);
            }
        }
        return 0;
    }
    case TYPE_DEVICE_LATENCY: {
        IAcousticEchoCanceler* echoCanceler = mEchoCanceler;
        if (echoCanceler) {
            LOGI("set device latency: %d", value);
            if (value > 0) {
                echoCanceler->setDeviceLatency(value);
            } else {
                echoCanceler->setDeviceLatency(0); // disable
            }
        }
        return 0;
    }

    case TYPE_NOISE_DETECT: {
       mUsingNoiseDetect = value;
       LOGI("Using noise detect : %d", value);
       return 0;
    }

    case TYPE_DENOISE_MODE: {
       mDenoiseMode = value;
       LOGI("Change denoise mode to : %d", value);
       return 1;
    }

    case TYPE_EXTRA_DECODER_GAIN: {
        int old = mStateDecoderGain;

        mStateDecoderGain = value;
        switch (mStateDecoderGain) {
        case LEVEL_MINUS_ENHANCED:
            opus_decoder_ctl(mDecoder, OPUS_SET_GAIN(-4096));
            break;
        case LEVEL_MINUS_STANDARD:
            opus_decoder_ctl(mDecoder, OPUS_SET_GAIN(-2048));
            break;
        case LEVEL_MINUS_MODERATE:
            opus_decoder_ctl(mDecoder, OPUS_SET_GAIN(-1024));
            break;
        case LEVEL_OFF:
            opus_decoder_ctl(mDecoder, OPUS_SET_GAIN(0));
            break;
        case LEVEL_MODERATE:
            opus_decoder_ctl(mDecoder, OPUS_SET_GAIN(512));
            break;
        case LEVEL_STANDARD:
            opus_decoder_ctl(mDecoder, OPUS_SET_GAIN(1024));
            break;
        case LEVEL_ENHANCED:
            opus_decoder_ctl(mDecoder, OPUS_SET_GAIN(2048));
            break;
        }

        if (old != value) {
            int gain = 0;
            opus_decoder_ctl(mDecoder, OPUS_GET_GAIN(&gain));
            LOGI("update decoder gain: %d, %d", mStateDecoderGain, gain);
        }

        return 0;
    }
    case TYPE_EXTRA_ENCODER_GAIN: {
        mStateEncoderGain = value;
        if (mStateEncoderGain == LEVEL_OFF) {
            AGC_RATIO_MIN = AGC_RATIO_MIN_DEFAULTS;
        }
        else {
            AGC_RATIO_MIN = 1.0f; // never drops
        }

        switch (mStateEncoderGain) {
        case LEVEL_MINUS_EXTREME:
            mEncoderGainfactor = 0.5f;
            break;
        case LEVEL_MINUS_ENHANCED:
            mEncoderGainfactor = 0.7f;
            break;
        case LEVEL_MINUS_STANDARD:
            mEncoderGainfactor = 0.8f;
            break;
        case LEVEL_MINUS_MODERATE:
            mEncoderGainfactor = 0.9f;
            break;
        case LEVEL_MODERATE:
            mEncoderGainfactor = 1.25f;
            break;
        case LEVEL_STANDARD:
            mEncoderGainfactor = 1.5f;
            break;
        case LEVEL_ENHANCED:
            mEncoderGainfactor = 2.0f;
            break;
        case LEVEL_EXTREME:
            mEncoderGainfactor = 3.0f;
            break;
        case LEVEL_OFF:
        default:
            mEncoderGainfactor = 1.0f;
            break;
        }

        LOGI("extra encoder gain: %d, factor: %f", value, mEncoderGainfactor);

        return 0;
    }
    case TYPE_DECODER_LPF: {
        if (mSampleRate == 16000) {
            mStateDecoderLPF = value;
            // LOGI("decoder lpf: %d", value);
        } else {
            LOGI("decoder lpf is disabled for 8KHz sample rate");
        }
        return 0;
    }
    case TYPE_ENCODER_LPF: {
        if (mSampleRate == 16000) {
            mStateEncoderLPF = value;
            // LOGI("encoder lpf: %d", value);
        } else {
            LOGI("encoder lpf is disabled for 8KHz sample rate");
        }
        return 0;
    }
    case TYPE_DECODER_NR: {
        mStateDecoderNR = value;
        // LOGI("decoder nr: %d", value);

        if (mNoiseSuppressionDecode) {
            switch (mStateDecoderNR) {
            case LEVEL_OFF:
                // nothing ...
                break;
            case LEVEL_MODERATE:
                IYCWebRtcNsx_set_policy(mNoiseSuppressionDecode, 0); // 0: Mild, 1: Medium , 2: Aggressive
                break;
            case LEVEL_STANDARD:
                IYCWebRtcNsx_set_policy(mNoiseSuppressionDecode, 1);
                break;
            case LEVEL_ENHANCED:
                IYCWebRtcNsx_set_policy(mNoiseSuppressionDecode, 2);
                break;
            }
        }
        else {
            mStateDecoderNR = LEVEL_OFF;
        }

        return 0;
    }
    case TYPE_ENCODER_NR: {
        mStateEncoderNR = value;
        // LOGI("encoder nr: %d", value);

        if (mNoiseSuppressionDecode) {
            switch (mStateDecoderNR) {
            case LEVEL_OFF:
                // nothing ...
                break;
            case LEVEL_MODERATE:
                IYCWebRtcNsx_set_policy(mNoiseSuppressionEncode, 0); // 0: Mild, 1: Medium , 2: Aggressive
                break;
            case LEVEL_STANDARD:
                IYCWebRtcNsx_set_policy(mNoiseSuppressionEncode, 1);
                break;
            case LEVEL_ENHANCED:
                IYCWebRtcNsx_set_policy(mNoiseSuppressionEncode, 2);
                break;
            }
        }
        else {
            mStateDecoderNR = LEVEL_OFF;
        }

        return 0;
    }
    case TYPE_DECODER_HPF: {
        return 0;
    }
    case TYPE_ENCODER_HPF: {
        mEncoderHPFLevel = value;
        LOGI("set HPF for encoder: %d", mEncoderHPFLevel);
        return 0;
    }
    }

    return -1;
}

int NativeVoiceCodec::getCodecState(jint typeID) {
    switch (typeID) {

// value types
    case TYPE_FRAME_SIZE: {
        return mStateFrameSize;
    }
    case TYPE_QUALITY: {
        return mStateQuality;
    }
    case TYPE_DIGITAL_AGC: {
        return mStateDigitalAGC;
    }
    case TYPE_EXPECTED_LOSS: {
        return mStateExpectedLoss;
    }
    case TYPE_DC_OFF_SET: {
        int dcOffsetValue = 0;
        int dcOffsetCount = -1;

// scan -100/+100 range for dc offset
        for (int i = 0; i < 25; i++) {
            int count1 = mVoiceStats[i & 0xffff];
            int count2 = mVoiceStats[(-i) & 0xffff];

            if (count1 > dcOffsetCount) {
                dcOffsetValue = i;
                dcOffsetCount = count1;
                LOGI("Voice Histogram: value=%d, count=%d, percent=%f", (short ) i, count1,
                     (count1 * 100.0f) / mVoiceCount);
            }

            if (count2 > dcOffsetCount) {
                dcOffsetValue = -i;
                dcOffsetCount = count2;
                LOGI("Voice Histogram: value=%d, count=%d, percent=%f", (short ) -i, count2,
                     (count2 * 100.0f) / mVoiceCount);
            }
        }

        LOGI("scan dc offset from recent session: value = %d, count = %d, total = %d", dcOffsetValue, dcOffsetCount,
             mVoiceCount);

        mStateDCOffset = dcOffsetValue;

        return mStateDCOffset;
    }
    // level types
    case TYPE_AEC: {
        return mStateAEC;
    }
    case TYPE_DEVICE_LATENCY: {
        int value = 0;
        IAcousticEchoCanceler* echoCanceler = mEchoCanceler;
        if (echoCanceler) {
            value = echoCanceler->getDeviceLatency();
            // LOGI("get device latency: %d", value);
        }
        return value;
    }
    case TYPE_EXTRA_DECODER_GAIN: {
        return mStateDecoderGain;
    }
    case TYPE_EXTRA_ENCODER_GAIN: {
        return mStateEncoderGain;
    }
    case TYPE_DECODER_LPF: {
        return mStateDecoderLPF;
    }
    case TYPE_ENCODER_LPF: {
        return mStateEncoderLPF;
    }
    case TYPE_DECODER_NR: {
        return mStateDecoderNR;
    }
    case TYPE_ENCODER_NR: {
        return mStateEncoderNR;
    }
    case TYPE_NOISE_DETECT: {
        return mUsingNoiseDetect;
    }
    case TYPE_DENOISE_MODE: {
        return mDenoiseMode;
    }

    //yangrui 0814
    case CFG_VALUE_XD_COHERENCE: {
            int value = 0;
            IAcousticEchoCanceler* echoCanceler = mEchoCanceler;
            if (echoCanceler) {
                value = echoCanceler->getXDCoherence();
           // LOGI("%s_%d, XDCoherence: %d",__FILE__,__LINE__, value);
            }
            return value;
    }
    case CFG_VALUE_XE_COHERENCE: {
            int value = 0;
            IAcousticEchoCanceler* echoCanceler = mEchoCanceler;
            if (echoCanceler) {
                value = echoCanceler->getXECoherence();

            }
          //  LOGI("XECoherence: %d", value);
            return value;
    }
    case CFG_VALUE_AEC_RATIO: {
                int value = 0;
                IAcousticEchoCanceler* echoCanceler = mEchoCanceler;
                if (echoCanceler) {
                    value = echoCanceler->getAECRatio();

                }
             //   LOGI("AECRatio: %d", value);
                return value;
     }
     }

    return 0;
}

// level 4
#define SMOOTH_RANGE_PLAY 4
inline short NativeVoiceCodec::smoothVoicePlay(short current) {
    mSmoothPlayHistory[mSmoothPlayPosition] = current;

    int d3 = mSmoothPlayHistory[(mSmoothPlayPosition + SMOOTH_RANGE_PLAY - 3) % SMOOTH_RANGE_PLAY];
    int d2 = mSmoothPlayHistory[(mSmoothPlayPosition + SMOOTH_RANGE_PLAY - 2) % SMOOTH_RANGE_PLAY];
    int d1 = mSmoothPlayHistory[(mSmoothPlayPosition + SMOOTH_RANGE_PLAY - 1) % SMOOTH_RANGE_PLAY];
    int d0 = current; // mSmoothPlayHistory[(mSmoothPlayPosition + SMOOTH_RANGE_PLAY - 0) % SMOOTH_RANGE_PLAY];

// // Method 1 f = d3 + 3 * d2 + 4 * d1 + 8 * d0
// int smoothed = (d3 + d2 + (d2 << 1) + (d1 << 2) + (d0 << 3)) >> 4; // divide by 16

// // Method 2 f = d3 + 2 * d2 + 3 * d1 + 10 * d0
// int smoothed = (d3 + (d2 << 1) + d1 + (d1 << 1) + (d0 << 1) + (d0 << 3)) >> 4; // divide by 16

// Method 3 f = d3 + d2 + 2 * d1 + 12 * d0
    int smoothed = (d3 + d2 + (d1 << 1) + (d0 << 2) + (d0 << 3)) >> 4; // divide by 16

// int smoothed = (d3 + d2 + (d1 << 1) + (d0 << 2)) >> 3; // divide by 8

// LOGI("pos=%d\td(n-3)=%d\td(n-2)=%d\td(n-1)=%d\td(n)=%d    \tsmoothed=%d", mSmoothPosition, d3, d2, d1, d0, smoothed);

    mSmoothPlayPosition = (mSmoothPlayPosition + 1) % SMOOTH_RANGE_PLAY;

    return smoothed;
}

#define SMOOTH_RANGE_RECORD 4
inline short NativeVoiceCodec::smoothVoiceRecord(short current) {
    mSmoothRecordHistory[mSmoothRecordPosition] = current;

    int d3 = mSmoothRecordHistory[(mSmoothRecordPosition + SMOOTH_RANGE_RECORD - 3) % SMOOTH_RANGE_RECORD];
    int d2 = mSmoothRecordHistory[(mSmoothRecordPosition + SMOOTH_RANGE_RECORD - 2) % SMOOTH_RANGE_RECORD];
    int d1 = mSmoothRecordHistory[(mSmoothRecordPosition + SMOOTH_RANGE_RECORD - 1) % SMOOTH_RANGE_RECORD];
    int d0 = current; // mSmoothRecordHistory[(mSmoothRecordPosition + SMOOTH_RANGE_RECORD - 0) % SMOOTH_RANGE_RECORD];

    int smoothed = (d3 + d2 + (d2 << 1) + (d1 << 2) + (d0 << 3)) >> 4; // divide by 16
// int smoothed = (d3 + d2 + (d1 << 1) + (d0 << 2)) >> 3; // divide by 8

// LOGI("pos=%d\td(n-3)=%d\td(n-2)=%d\td(n-1)=%d\td(n)=%d    \tsmoothed=%d", mSmoothPosition, d3, d2, d1, d0, smoothed);

    mSmoothRecordPosition = (mSmoothRecordPosition + 1) % SMOOTH_RANGE_RECORD;

    return smoothed;
}

//// level 6
//inline short NativeVoiceCodec::smoothVoice(short current) {
//    mSmoothHistory[mSmoothPosition] = current;
//
//    int d5 = mSmoothHistory[(mSmoothPosition + mSmoothRange - 5) % mSmoothRange];
//    int d4 = mSmoothHistory[(mSmoothPosition + mSmoothRange - 4) % mSmoothRange];
//    int d3 = mSmoothHistory[(mSmoothPosition + mSmoothRange - 3) % mSmoothRange];
//    int d2 = mSmoothHistory[(mSmoothPosition + mSmoothRange - 2) % mSmoothRange];
//    int d1 = mSmoothHistory[(mSmoothPosition + mSmoothRange - 1) % mSmoothRange];
//    int d0 = current; // mSmoothHistory[(mSmoothPosition + mSmoothRange - 0) % mSmoothRange];
//
//    int smoothed = (d5 + d4 + d3 + (d2 << 1) + d1 + (d1 << 1) + (d0 << 3)) >> 4; // divide by 16
//
//    // LOGI("pos=%d\td(n-3)=%d\td(n-2)=%d\td(n-1)=%d\td(n)=%d    \tsmoothed=%d", mSmoothPosition, d3, d2, d1, d0, smoothed);
//
//    mSmoothPosition = (mSmoothPosition + 1) % mSmoothRange;
//
//    return smoothed;
//}

// ----------------------------------------------------------------------------------------------

int NativeVoiceCodec::doHowlingDetect(short* input, int size)
{
    int type = doFeedbackDetect(input, size, this->fdStatus, mHowling);
    switch (type) {
    case Normal: {
        if (this->fdStatus) {
            if (this->isMute == true) {
                this->isMute = false;
//                    LOGI("Howling: 无啸叫，自动取消静音");
            }
        }
        this->fdStatus = 0;
    }
    break;

    case Nochange:
    {
    } break;

    case Feedback: {
        if (!this->fdStatus) {
            if (this->isMute == false) {
                this->isMute = true;
//                    LOGI("Howling: 啸叫出现，自动静音");
            }
        }
        this->fdStatus = 1;
    }
    break;

    default:
        break;
    }
    return type;
}

