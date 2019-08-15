// ----------------------------------------------------------------------------------------------

#include "NativeOpusCodec.h"

#define abs(x) (((x) > 0) ? (x) : -(x))

static const float AGC_RATIO_MAX = 4.0f;
static const float AGC_RATIO_MIN = 0.5f;
static const float AGC_TARGET_PERCENT = 32768 * 0.7f;

// ----------------------------------------------------------------------------------------------

NativeOpusCodec::NativeOpusCodec()
        : mSampleRate(0), mCompression(0), mOpenRef(0), mEncoder(NULL), mDecoder(NULL), mNoiseSuppressionEncode(NULL), mNoiseSuppressionDecode(
                NULL), mShouldNormalize(false), mPrevSample(0) {

    memset(mSmoothPlayHistory, 0, sizeof(mSmoothPlayHistory));
    mSmoothPlayPosition = 0; // reset position

    memset(mSmoothRecordHistory, 0, sizeof(mSmoothPlayHistory));
    mSmoothRecordPosition = 0;
}

NativeOpusCodec::~NativeOpusCodec() {
}

// ----------------------------------------------------------------------------------------------

int NativeOpusCodec::open(int sampleRate, int compression, int flags) {
    if (mOpenRef) {
        LOGE("error, codec already opened");
        return -1;
    }

    LOGI("opusOpen: sampleRate=%d, compression=%d, flags=%d", sampleRate, compression, flags);

    int ret = 0;
    int tmp = 0;

    mOpenRef++;

    // save
    mOpenFlags = flags;
    mSampleRate = sampleRate;
    mCompression = compression;

    mEnableExtraGain = (mOpenFlags & FLAG_EXTRA_GAIN) == FLAG_EXTRA_GAIN;
    mEnableLowPassOnRecording = (mOpenFlags & FLAG_RECORDER_LPF) == FLAG_RECORDER_LPF;

    int result = 0;

    // setup sample rate
    if (sampleRate == 48000 || sampleRate == 24000 || sampleRate == 16000 || sampleRate == 12000
            || sampleRate == 8000) {
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
    }
    else {
        return -1;
    }

    mOpusFrameSize = mSampleRate / 50; // 20ms frame length

    // enable features: set bitrate, eanble vbr
    opus_encoder_ctl(mEncoder, OPUS_SET_VBR(1)); // default, not necessary
    opus_encoder_ctl(mEncoder, OPUS_SET_DTX(1));
    opus_encoder_ctl(mEncoder, OPUS_SET_INBAND_FEC(1));
    opus_encoder_ctl(mEncoder, OPUS_SET_COMPLEXITY(0)); // CPU on Nexus One: 0: 33%; 4: 40%; 6: 45%; 10: 50%
    opus_encoder_ctl(mEncoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(mEncoder, OPUS_SET_PACKET_LOSS_PERC(15));
    opus_encoder_ctl(mEncoder, OPUS_SET_PREDICTION_DISABLED(0)); // default, not necessary
    opus_encoder_ctl(mEncoder, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_MEDIUMBAND));

    // finally, setup compression
    setQuality(compression);

    if (sampleRate == 32000 || sampleRate == 16000 || sampleRate == 8000) {
        ret = IYCWebRtcNsx_Create(&mNoiseSuppressionEncode);
        if (ret != 0) {
            LOGE("ERROR: could not perform noise suppression");
            mNoiseSuppressionEncode = NULL;
            mNoiseSuppressionDecode = NULL;
            return ret;
        }

        ret = IYCWebRtcNsx_Create(&mNoiseSuppressionDecode);
        if (ret != 0) {
            LOGE("ERROR: could not perform noise suppression");
            mNoiseSuppressionEncode = NULL;
            mNoiseSuppressionDecode = NULL;
            return ret;
        }

        mNSFrameSize = mSampleRate / 100; // 10ms frame length
        IYCWebRtcNsx_Init(mNoiseSuppressionEncode, mSampleRate);
        IYCWebRtcNsx_set_policy(mNoiseSuppressionEncode, 2);

        IYCWebRtcNsx_Init(mNoiseSuppressionDecode, mSampleRate);
        IYCWebRtcNsx_set_policy(mNoiseSuppressionDecode, 0);
    }
    else {
        mNoiseSuppressionEncode = NULL;
        mNoiseSuppressionDecode = NULL;
    }

    mDcOffset = 0;
    mVoiceCount = 0;
    memset(mVoiceStats, 0, sizeof(mVoiceStats));

    return 0;
}

int NativeOpusCodec::close() {
    if (!mOpenRef) {
        LOGE("Error: codec not opened. - close");
        return 32768;
    }

    mOpenRef--;

    LOGI("opusClose");

    opus_encoder_destroy(mEncoder);
    opus_decoder_destroy(mDecoder);

    WebRtcNsx_Free(mNoiseSuppressionEncode);
    WebRtcNsx_Free(mNoiseSuppressionDecode);

    mEncoder = NULL;
    mDecoder = NULL;

    return mVoiceAgcRange;
}

int NativeOpusCodec::setQuality(int compression) {
    if (!mOpenRef) {
        LOGE("Error: codec not opened. setQuality");
        return mCompression;
    }

    if (compression <= 0) {
        compression = 0;
    }
    else if (compression >= 10) {
        compression = 10;
    }

    int old = mCompression;

    mCompression = compression;

    // 2G:      q=1  9.0Kbps (8KHZ) 3KB/s dual way session
    // 3G/4G:   q=3 15.0Kbps (8KHZ) 4KB/s
    // Wifi:    q=7 21.0Kbps (8KHZ) 6KB/s
    int bitRate = (7000 + 2000 * mCompression) * (mSampleRate / 8000);

    opus_encoder_ctl(mEncoder, OPUS_SET_BITRATE(bitRate));
    opus_encoder_ctl(mEncoder, OPUS_SET_VBR(1));

    LOGI("opusSetQuality: compress=%d sampleRate=%d, bps=%d bits/s", compression, mSampleRate, bitRate);

    return old;
}

int NativeOpusCodec::setGain(int gain) {
    if (!mOpenRef || !mDecoder) {
        LOGE("Error: codec not opened. - setGain");
        return -1;
    }

    if (gain > 32768 || gain < -32768) {
        return -1;
    }

    opus_decoder_ctl(mDecoder, OPUS_SET_GAIN(gain));

    return 0;
}

int NativeOpusCodec::setAGC(int initialNormal) {
    if (!mOpenRef || !mDecoder) {
        LOGE("Error: codec not opened. - set adaptive normalize");
        return -1;
    }

    if (abs(initialNormal) > 32768) {
        return -1;
    }

    int old = mVoiceAgcRange;

    mShouldNormalize = initialNormal > 0;

    if (mShouldNormalize) {
        // LOGI("enable digital agc: %d", initialNormal);
        mVoiceAgcRange = initialNormal;  // initial range

        mOutsiderCount = 0;
        mInsiderCount = 0;

        mOutsiderLimit = mEnableExtraGain ? 40 : 10; // drop gain fast, but increase slower
        mInsiderLimit = mSampleRate; // 8KHz = 8000, 16KHz = 16000

        mNormalizeFactor = AGC_TARGET_PERCENT / mVoiceAgcRange;

        LOGI("setAdaptiveNormalize: range=%d normalize factor: %f", mVoiceAgcRange, mNormalizeFactor);
    }
    else {
        LOGI("do not enable digital agc");
        mVoiceAgcRange = 0;
    }

    return old;
}

int NativeOpusCodec::getAGC() {
    if (!mOpenRef) {
        LOGE("Error: codec not opened. - get adaptive normalize");
        return -1;
    }

    return mVoiceAgcRange;
}

int NativeOpusCodec::setExpectedLossRate(int percent) {
    if (!mOpenRef || !mEncoder) {
        LOGE("Error: codec not opened. - set expected loss rate");
        return -1;
    }

    if (percent < 5) {
        percent = 5;
    }

    if (percent > 60) {
        percent = 60;
    }

    opus_encoder_ctl(mEncoder, OPUS_SET_PACKET_LOSS_PERC(percent));

    return 0;
}

int NativeOpusCodec::getFrameSize() {
    if (!mOpenRef) {
        LOGE("Error: codec not opened. - get frame size");
        return -1;
    }

    return 160;
}

int NativeOpusCodec::setDcOffset(int initialOffset) {
    if (!mOpenRef) {
        LOGE("Error: codec not opened. - set dc offset");
        return 0;
    }

    if (!mShouldNormalize) {
        LOGE("error: agc not enabled, do not allow set dc offset");
        return 0;
    }

    int old = mDcOffset;
    mDcOffset = initialOffset; // not updated during voice session
    LOGI("set dc offset: %d", mDcOffset);

    return old;
}

int NativeOpusCodec::getDcOffset() {
    if (!mOpenRef) {
        LOGE("Error: codec not opened. - get dc offset");
        return 0;
    }

    if (!mShouldNormalize) {
        // no normalize performed
        return 0;
    }

    int dcOffsetValue = 0;
    int dcOffsetCount = -1;

    for (int i = -32768; i < 32768; i++) {
        int count = mVoiceStats[i & 0xffff];
        if (count > dcOffsetCount) {
            dcOffsetValue = i;
            dcOffsetCount = count;
            // LOGI("Voice Histogram: value=%d, count=%d, percent=%f", (short )i, count, (count * 100.0f) / mVoiceCount);
        }
    }

    LOGI("scaned dc offset from recent session: value = %d, count = %d", dcOffsetValue, dcOffsetCount);

    return dcOffsetValue;
}

// ----------------------------------------------------------------------------------------------

int NativeOpusCodec::encode(JNIEnv *env, jshortArray voiceIn, jint offset, jbyteArray voiceOut, jint nrOfSamples) {
    if (!mOpenRef) {
        LOGE("Error: codec not opened. - encode");
        return -1;
    }

    if (mOpusFrameSize != nrOfSamples || offset != 0) {
        LOGE("Error: nrOfSamples too large - %d", nrOfSamples);
        return -1;
    }

    env->GetShortArrayRegion(voiceIn, 0, nrOfSamples, mEncoderBufferRaw);

    // LOGI("Encode: inLen=%d, agc=%d", nrOfSamples, mShouldNormalize);

    if (mShouldNormalize) { // perform audio normalization
        int NOISE_GATE_LOW = 3;
        int NOISE_GATE_HIGH = 32000;

        for (int i = 0; i < nrOfSamples; i++) {
            short valueShortRAW = mEncoderBufferRaw[i];
            short valueShort = valueShortRAW + mDcOffset;
            short valueShortABS = abs(valueShort);

            if (valueShortABS > NOISE_GATE_LOW && valueShortABS < NOISE_GATE_HIGH) {
                // ignore pure silent
                if (valueShortABS > mVoiceAgcRange) {
                    mOutsiderCount++;
                }
                else {
                    mInsiderCount++;
                }

                if (mNormalizeFactor > AGC_RATIO_MIN && mOutsiderCount > mOutsiderLimit) {
                    // range is too small, enlarge it & perform normalize
                    mOutsiderCount = 0;
                    mInsiderCount = 0;

                    mVoiceAgcRange += 400;

                    mNormalizeFactor = AGC_TARGET_PERCENT / mVoiceAgcRange;

                    if (mNormalizeFactor < AGC_RATIO_MIN) {
                        mNormalizeFactor = AGC_RATIO_MIN;
                    }
                    else if (mNormalizeFactor > AGC_RATIO_MAX) {
                        mNormalizeFactor = AGC_RATIO_MAX;
                    }

                    // LOGE("AGC ++ : factor=%f, max=%d, offset=%d", mNormalizeFactor, mVoiceAgcRange, mDcOffset);
                }
                else if (mNormalizeFactor < AGC_RATIO_MAX && mInsiderCount > mInsiderLimit) { // at least 2 seconds of audio data
                // range is too large, shrink it and enlarge volumn
                    mOutsiderCount = 0;
                    mInsiderCount = 0;

                    mVoiceAgcRange -= 100;

                    mNormalizeFactor = AGC_TARGET_PERCENT / mVoiceAgcRange;

                    if (mNormalizeFactor < AGC_RATIO_MIN) {
                        mNormalizeFactor = AGC_RATIO_MIN;
                    }
                    else if (mNormalizeFactor > AGC_RATIO_MAX) {
                        mNormalizeFactor = AGC_RATIO_MAX;
                    }

                    // LOGE("AGC -- : factor=%f, max=%d, offset=%d", mNormalizeFactor, mVoiceAgcRange, mDcOffset);
                }
            }

            // normalize offset values
            if (valueShortABS >= 1) {
                if (mEnableExtraGain) {
                    mEncoderBufferRaw[i] = valueShort * mNormalizeFactor * 2.0f;
                } else {
                    mEncoderBufferRaw[i] = valueShort * mNormalizeFactor;
                }
            }

            // smooth if extra gain applied: scratch sound will be heard if not do this
            if (mEnableExtraGain || mEnableLowPassOnRecording) {
                int old =  mEncoderBufferRaw[i];
                mEncoderBufferRaw[i] = mPrevSample * 0.4f +  old * 0.6f;
                mPrevSample = old;
            }

            // count with raw values
            int value = (valueShortRAW) & 0xffff;
            mVoiceStats[value]++;
        }

        mVoiceCount += nrOfSamples;
    }

    // noise suppression: 80 samples per action
    int nsOk = 1;
    //if (mNoiseSuppressionEncode) {
    if ( 0 )
    {
        for (int i = 0; i < nrOfSamples; i += mNSFrameSize) {
            // accept 10ms frame only
            int ret = IYCWebRtcNsx_Process(mNoiseSuppressionEncode, mEncoderBufferRaw + i, NULL,
                    mEncoderBufferNoiseSuppressed + i, NULL);
            if (ret < 0) {
                LOGI("ns ret=%d", ret);
                nsOk = 0;
                break;
            }
        }

//        for (int i = 0; i < nrOfSamples; i++) {
//            mEncoderBufferNoiseSuppressed[i] = smoothVoicePlay(mEncoderBufferNoiseSuppressed[i]);
//        }
    }
    else {
        nsOk = 0;
    }

    // encode to opus: accept multiple of 2.5ms frame
    int totalBytes = opus_encode(mEncoder, (nsOk ? mEncoderBufferNoiseSuppressed : mEncoderBufferRaw), nrOfSamples,
            mEncoderBufferEncoded, 4096);

    if (totalBytes > 0) {
        env->SetByteArrayRegion(voiceOut, 0, totalBytes, (const signed char*) mEncoderBufferEncoded);
    }
    else {
        LOGE("Error: encode error: %d", totalBytes);
    }

    // LOGI("Encoding: %d frames --> total: %d bytes", nrOfSamples, totalBytes);

    return (jint) totalBytes;
}

int NativeOpusCodec::decode(JNIEnv *env, jbyteArray input, jshortArray output, jint size) {
    if (!mOpenRef) {
        LOGE("Error: codec not opened. - decode");
        return -1;
    }

    int decodedLen = 0;
    if (input == NULL || size <= 0) {
        // insert fake frame by decoder
        decodedLen = opus_decode(mDecoder, (unsigned char*) NULL, 0, (opus_int16 *) mDecoderBufferOutput,
                mOpusFrameSize, 1);
    }
    else {
        env->GetByteArrayRegion(input, 0, size, mDecoderBufferInput);

        decodedLen = opus_decode(mDecoder, (unsigned char*) mDecoderBufferInput, size,
                (opus_int16 *) mDecoderBufferOutput, mOpusFrameSize, 0);

        // LOGI("Decode: encoded=0x%08x, decoded=0x%08x, size=%d, decodedLen=%d", (void* )input, (void* )output, size,
        //         decodedLen);

        // LOGI("Decode: size=%d, decodedLen=%d", size, decodedLen);
    }

    if (decodedLen > 0) {
        int nsOk = 0;

        nsOk = 1;
        for (int i = 0; i < decodedLen; i++) {
            mDecoderBufferNoiseSuppressed[i] = smoothVoicePlay(mDecoderBufferOutput[i]);
        }

        // LOGD("decoded: length=%d, nrOfSamples=%d", decodedLen, nrOfSamples);

        // LOGI("Decoded Length=%d", decodedLen);
        env->SetShortArrayRegion(output, 0, decodedLen,
                (nsOk == 1) ? mDecoderBufferNoiseSuppressed : mDecoderBufferOutput);
    }
    else {
        LOGE("Error: decode error: %d", decodedLen);
        decodedLen = 0;
    }

    return (jint) decodedLen;
}

// level 4
#define SMOOTH_RANGE_PLAY 4
inline short NativeOpusCodec::smoothVoicePlay(short current) {
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
inline short NativeOpusCodec::smoothVoiceRecord(short current) {
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
//inline short NativeOpusCodec::smoothVoice(short current) {
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
