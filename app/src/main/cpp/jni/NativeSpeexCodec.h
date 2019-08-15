#ifndef NATIVE_SPEEX_CODEC_0B3C2E33C4C74C769A6DED78D7B077A9
#define NATIVE_SPEEX_CODEC_0B3C2E33C4C74C769A6DED78D7B077A9

// ----------------------------------------------------------------------------------------------

#include <jni.h>

#include <string.h>
#include <unistd.h>

#include <speex/speex.h>

#include "logcat.h"
#include "com_zayhu_library_jni_Speex.h"

// ----------------------------------------------------------------------------------------------

class NativeSpeexCodec
{
public:
    NativeSpeexCodec();
    ~NativeSpeexCodec();

    int open(int sampleRate, int compression);
    void close();

    int setQuality(int compression);
    int getFrameSize();

    int encode(JNIEnv *env, jshortArray lin, jint offset, jbyteArray encoded, jint size);
    int decode(JNIEnv *env, jbyteArray encoded, jshortArray lin, jint size);

private:
    int mSampleRate;
    int mCompression;

    int mOpenRef;

    int mFrameSizeDecode;
    int mFrameSizeEncode;

    SpeexBits mSpeexBitsEncode;
    SpeexBits mSpeexBitsDecode;

    void *mStateEncode;
    void *mStateDecode;
};

#endif // NATIVE_SPEEX_CODEC_0B3C2E33C4C74C769A6DED78D7B077A9
