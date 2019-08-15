#ifndef NATIVE_OPUS_CODEC_F10EF03B75F54C30AE57A9BAB127F4F8
#define NATIVE_OPUS_CODEC_F10EF03B75F54C30AE57A9BAB127F4F8

// ----------------------------------------------------------------------------------------------

#include <jni.h>

#include <string.h>
#include <unistd.h>

#include <opus.h>

//#include "IYCWebRtcNSx.h"
#include "/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/YCInterface/IYCWebRtcNSx.h"
#include "logcat.h"

#include "noisedetect.h"

// ----------------------------------------------------------------------------------------------

static const int FLAG_ADAPTIVE_NORMALIZE = 1;
static const int FLAG_EXTRA_GAIN = 2;
static const int FLAG_RECORDER_LPF = 4;

class NativeOpusCodec
{
public:
    NativeOpusCodec();
    ~NativeOpusCodec();

    int open(int sampleRate, int compression, int flags);
    int close();

    int setQuality(int compression);
    int setGain(int gain);
    int setExpectedLossRate(int percent);

    int setAGC(int initialNormal);
    int getAGC();

    int setDcOffset(int initialOffset);
    int getDcOffset();

    int getFrameSize();

    int encode(JNIEnv *env, jshortArray lin, jint offset, jbyteArray encoded, jint size);
    int decode(JNIEnv *env, jbyteArray encoded, jshortArray lin, jint size);

private:
    int mSampleRate;
    int mCompression;

    int mOpenRef;

    int mOpenFlags;

    int mEnableExtraGain;
    int mEnableLowPassOnRecording;

    int mOpusFrameSize;
    OpusEncoder* mEncoder;
    OpusDecoder* mDecoder;

    jshort mEncoderBufferRaw[2048];
    jshort mEncoderBufferNoiseSuppressed[2048];
    unsigned char mEncoderBufferEncoded[4096];

    jbyte mDecoderBufferInput[4096];
    jshort mDecoderBufferNoiseSuppressed[2048];
    jshort mDecoderBufferOutput[2048];

    int mNSFrameSize;
    NsxHandle *mNoiseSuppressionEncode;
    NsxHandle *mNoiseSuppressionDecode;

    int mPrevSample;

    int mDcOffset;
    int mVoiceCount;
    int mVoiceStats[65536];

    int mOutsiderCount;
    int mInsiderCount;

    int mOutsiderLimit;
    int mInsiderLimit;

    int mVoiceAgcRange;

    bool mShouldNormalize;
    float mNormalizeFactor;

    int mSmoothPlayHistory[16];
    int mSmoothPlayPosition;

    int mSmoothRecordHistory[16];
    int mSmoothRecordPosition;

    short smoothVoicePlay(short current);
    short smoothVoiceRecord(short current);
};

#endif // NATIVE_OPUS_CODEC_F10EF03B75F54C30AE57A9BAB127F4F8
