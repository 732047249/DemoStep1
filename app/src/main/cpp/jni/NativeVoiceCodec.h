#ifndef NATIVE_VOICE_CODEC_4f43ce2bb0f37f49e7945fe6ba76d62f
#define NATIVE_VOICE_CODEC_4f43ce2bb0f37f49e7945fe6ba76d62f

// ----------------------------------------------------------------------------------------------

#include <jni.h>

#include <string.h>
#include <unistd.h>

#include <opus.h>

#include "IYCWebRtcAgc.h"
#include "IYCWebRtcNSx.h"
#include "IYCWebRtcAEC.h"
#include "IYCWebRtcAECM.h"

#include "IAcousticEchoCanceler.h"

#include "NativeWebRtcFilter.h"
#include "feedbackDetect.h"

#include "rnnoise.h"

#include "noisedetect.h"

class HighPassFilter;

// ----------------------------------------------------------------------------------------------

static const int CODEC_OPUS = 0x01;
static const int CODEC_SPEEX = 0x02; // not supported currently

// ----------------------------------------------------------------------------------------------

static const int AEC_TYPE_RESET = -1;
static const int AEC_TYPE_DISABLE = 0;
static const int AEC_TYPE_FULL = 1;
static const int AEC_TYPE_MOBILE = 2;

// ----------------------------------------------------------------------------------------------

static const int LEVEL_OFF = 0;
static const int LEVEL_MODERATE = 1;
static const int LEVEL_STANDARD = 2;
static const int LEVEL_ENHANCED = 3;
static const int LEVEL_EXTREME = 4;

static const int LEVEL_MINUS_MODERATE = -1;
static const int LEVEL_MINUS_STANDARD = -2;
static const int LEVEL_MINUS_ENHANCED = -3;
static const int LEVEL_MINUS_EXTREME = -4;

// ------------------------------------------------------------------------------------------------------------

// value types
static const int TYPE_FRAME_SIZE = 0x0100;
static const int TYPE_QUALITY = 0x0101;
static const int TYPE_DIGITAL_AGC = 0x0102;
static const int TYPE_EXPECTED_LOSS = 0x0103;
static const int TYPE_DC_OFF_SET = 0x0104;
static const int TYPE_AEC = 0x0105; // disable = 0, CODEC_AEC, CODEC_AEC_MOBILE
static const int TYPE_HOWLING = 0x0106;  // disable = 0, enable = use howling detect
static const int TYPE_DEVICE_LATENCY = 0x0107;  // disable <= 0, otherwise is value in ms
static const int TYPE_NOISE_DETECT = 0x0108; // disable <= 0, otherwise using noise detect
static const int TYPE_DENOISE_MODE = 0x0109; // DENOISE_MODE_NONE, DENOISE_MODE_INTELLIGENCE, DENOISE_MODE_WEBRTC


//yangrui 0814
#define CFG_VALUE_XD_COHERENCE          0x30010
#define CFG_VALUE_XE_COHERENCE          0x30011
#define CFG_VALUE_AEC_RATIO             0x30012
#define CFG_ACTION_HANDS_FREE           0x10004
// ------------------------------------------------------------------------------------------------------------

// level types
static const int TYPE_EXTRA_DECODER_GAIN = 0x0201;
static const int TYPE_EXTRA_ENCODER_GAIN = 0x0202; // extra gain, out of digital agc

static const int TYPE_DECODER_LPF = 0x0203;
static const int TYPE_ENCODER_LPF = 0x0204;

static const int TYPE_DECODER_NR = 0x0205;
static const int TYPE_ENCODER_NR = 0x0206;

static const int TYPE_DECODER_HPF = 0x0207;
static const int TYPE_ENCODER_HPF = 0x0208;

// ------------------------------------------------------------------------------------------------------------
// DSP filter max input buffer size
static const int FILTER_MAX_INPUT_SIZE = 4096;

static const int DENOISE_MODE_NONE = 1;
static const int DENOISE_MODE_INTELLIGENCE = 2;
static const int DENOISE_MODE_WEBRTC = 3;

#define  FRAME_COUNT_IN_WINDOW  100
#define  FRAME_SIZE_SAMPLERATE_16k  320
#define  LB_SUBBAND_IDX  31  // according to 1kHz

#define FRAME_SIZE 320

class NativeVoiceCodec
{
public:
    NativeVoiceCodec();
    ~NativeVoiceCodec();

    int open(int codec, int sampleRate, int flags);
    int close();

    int decode(jbyte* encodedData, int encodedLen, jshort* decodedData, int maxDecodeLen, int decodeFEC);
    int encode(JNIEnv * env, jshort* farEndData, jshort* nearEndData, int msInSndCardBuf, jbyte* encodedData, int maxEncodeLen);

    int setCodecState(jint typeID, jint value);
    int getCodecState(jint typeID);

    int doHowlingDetect(short* input, int size);


private:
    // ----------------------------------------------------------------------------------------------
    // flags & internal state

    int mOpenRef;

    int mCodecId;
    int mSampleRate;
    int mOpenFlags;

    // value types: public state
    int mStateFrameSize;
    int mStateQuality;
    int mStateDigitalAGC;
    int mStateExpectedLoss;
    int mStateDCOffset;

    // level types: public state
    int mStateAEC;
    int mStateDecoderGain;
    int mStateEncoderGain;
    int mStateDecoderLPF;
    int mStateEncoderLPF;
    int mStateDecoderNR;
    int mStateEncoderNR;

    float mEncoderGainfactor;

    // ----------------------------------------------------------------------------------------------

    int mStateNSFrameSize; // internal state for ns
    int mStateAgcFrameSize; // internal state for agc

    // ----------------------------------------------------------------------------------------------
    // decoder related
    OpusDecoder* mDecoder;
    NsxHandle *mNoiseSuppressionDecode;

    // jbyte mDecoderBufferInput[4096];
    // jshort mDecoderBufferNoiseSuppressed[2048];
    jshort mDecoderBufferOutput[2048];

    int mDecoderLPFPrevSample;

    // ----------------------------------------------------------------------------------------------
    // encoder related
    OpusEncoder* mEncoder;
    NsxHandle *mNoiseSuppressionEncode;
    void* mAGC;
    int mStateMicLevel;

    jshort mEncoderBufferRaw[2048];
    // jshort mEncoderBufferNoiseSuppressed[2048];
    // unsigned char mEncoderBufferEncoded[4096];

    IAcousticEchoCanceler* mEchoCanceler;
    IAcousticEchoCanceler* mEchoCancelerPendingDelete;

    int mEncoderLPFPrevSample;

    HighPassFilter* mEncoderHPF;

    // ----------------------------------------------------------------------------------------------

    int mEncodedFrameCount;
    int mDecodedFrameCount;

    // DC offset
    int mVoiceCount;
    int mVoiceStats[65536];

    // AGC
    int mOutsiderCount;
    int mInsiderCount;
    int mOutsiderLimit;
    int mInsiderLimit;
    float mNormalizeFactor;

    // LPF
    int mSmoothPlayHistory[16];
    int mSmoothPlayPosition;

    int mSmoothRecordHistory[16];
    int mSmoothRecordPosition;

    short smoothVoicePlay(short current);
    short smoothVoiceRecord(short current);

    // dsp filter, since filter use last frame state,
    // we need to use two filter separately
    bool mUseDspFilter;
    NativeWebRtcFilter * mWebRtcEncodeFilter;
    short mWebRtcFilterEncodeResult[FILTER_MAX_INPUT_SIZE];
    NativeWebRtcFilter * mWebRtcDecodeFilter;
    short mWebRtcFilterDecodeResult[FILTER_MAX_INPUT_SIZE];

    // HPF for recording
    int mEncoderHPFLevel;

    int isUsingHowlingDetect;
    struct feedbackDetect *mHowling;
    int fdStatus;
    bool isMute;

    int mUsingNoiseDetect;
    int mLastNoiseDetectRet;
    int nFrameCount;

    jclass mClazzVoiceCodec;
    jmethodID mMethodNoiseDetectID;
    jmethodID mMethodNoiseReportID;

    //yangrui  output pcm data
   // FILE *fNear;
  //  FILE *fFar;
  //  FILE *fOut;
   // rnnoise
   DenoiseState *mDenoiseState;
   float mFrameData[FRAME_SIZE];
   short mNSBuffer[FRAME_SIZE];

   int mDenoiseMode;
   float fNoiseReduceTotal;
   float fNoiseReduceAver;
   float fNoiseRatioBefore;
   float fNoiseRatioAfter;
   float fNoiseRatioTotal;
   float fNoiseRatioAver;

};

#endif // NATIVE_VOICE_CODEC_4f43ce2bb0f37f49e7945fe6ba76d62f

