//
// Created by Qiongzhu Wan on 16-9-28.
//

#ifndef NATIVE_AUDIO_YEECALLRECORDER_H
#define NATIVE_AUDIO_YEECALLRECORDER_H

#include <assert.h>
#include <jni.h>
#include <string.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

// for lock & synchronization
#include <pthread.h>

#include "NativeWebRtcAEC.h"
#include "NativeWebRtcAECM.h"

#include "CSignal.h"

// 20ms audio frame size
#define RECORDER_DURATION_MS (20)

// 200 * 20ms = 4 seconds buffer. actually 2 ~ 4 buffer is enough for use. we allocate more for for bad cases
#define BUFFER_COUNT_MAX (200)

// ----------------------------------------------------------------------------------------------------
// flags for recorder
#define FLAGS_DEBUG_MODE            (1 << 0)
#define FLAGS_DEBUG_MODE_VERBOSE    (1 << 1)
#define FLAGS_DEBUG_LOOP_REC_PLAY   (1 << 2)

// ----------------------------------------------------------------------------------------------------
// config for recorder.
// range [0x0000 ~ 0xffff] is allocated to NativeVoiceCodec. so we use higher range for recorder related actions

// config type 1: user interactive actions
#define CFG_ACTION_MUTE_RECORD          0x10001
#define CFG_ACTION_MUTE_PLAY            0x10002
#define CFG_ACTION_MUTE_ALL             0x10003
#define CFG_ACTION_HANDS_FREE           0x10004
#define CFG_ACTION_MEASURE_LATENCY      0x10007
#define CFG_ACTION_NOISE_DETECT         0x10008
#define CFG_ACTION_DENOISE_MODE         0x10009

// config type 2: boolean config, on/off

// config type 3: int value config
#define CFG_VALUE_RECORDED              0x30001
#define CFG_VALUE_PLAYED                0x30002
#define CFG_VALUE_JITTER                0x30003
#define CFG_VALUE_PLAY_QUEUE            0x30004
#define CFG_VALUE_RECORD_PLAY_OFFSET    0x30005
#define CFG_VALUE_RECORD_PRE_PROCESS    0x30006
#define CFG_VALUE_SAMPLES_PER_FRAME     0x30007
#define CFG_VALUE_DEVICE_LATENCY        0x30008
#define CFG_VALUE_LIVE_LATENCY          0x30009

//yangrui 0814
#define CFG_VALUE_XD_COHERENCE          0x30010
#define CFG_VALUE_XE_COHERENCE          0x30011
#define CFG_VALUE_AEC_RATIO             0x30012


// ----------------------------------------------------------------------------------------------------

#define POST_DECODE_ACTION_NONE             -1

// #define POST_DECODE_ACTION_DROP_ALL         001

#define POST_DECODE_ACTION_SKIP_SILENCE         101
#define POST_DECODE_ACTION_SKIP_ANY             102
#define POST_DECODE_ACTION_FAST_FORWARD         103

#define POST_DECODE_ACTION_REWIND_SILENCE       201

#define POST_DECODE_ACTION_RESAMPLE_SPEEDUP1    301
#define POST_DECODE_ACTION_RESAMPLE_SPEEDUP2    302
#define POST_DECODE_ACTION_RESAMPLE_SPEEDUP3    303
#define POST_DECODE_ACTION_RESAMPLE_SPEEDUP4    304

// ----------------------------------------------------------------------------------------------------

class NativeVoiceCodec;
class latencyMeasurer;

// maximum pending frames for play back
#define PENDING_FRAME_COUNT (128)

// disable fast play for 100 frames (2sec) if jitter found during playback
#define JITTER_RECOVERY_COUNT (100)

// lack of playback data, minimal request gap
#define REQUEST_PLAYBACK_FRAME_COUNT (10)

// generate an playback event every 25 frames == 0.5s
#define SEND_PLAYBACK_EVENTS_FRAME_COUNT (25)

// generate an no traffic event every 300 frames == 6s for continuous lack of playback data
#define SEND_PLAYBACK_NO_TRAFFIC_FRAME_COUNT (300)

// average the latest record/play frame intervals, send non muted voice to peer only if interval is stable enough
#define STABLE_VOICE_COUNT      (20)

// entering a recovery process if stall level greater than 5
#define STALL_LEVEL_LIMIT       (5)

// recovery takes 10 frame time to enter stable state
#define STALL_RECOVERY          (10)

#define LATENCY_SLOT            (125)

struct PlayFrame {
    SLint32 frameId;
    SLint32 length;
    jbyte data[2040]; // 2048 - 8 = 2040 bytes, sizeof(PlayFrame) = 2048, 128 entries = 256KB buffer
};

class YeeCallRecorder {
public:
    // ctor & dtor
    YeeCallRecorder();

    virtual ~YeeCallRecorder();

public:
    bool create(int sampleRate, int recordBufferSize, int flags, int recordFlags, int playFlags, int recPlayOffsetMS);      // allocate opensl engine objects

    void destroy();     // destroy all

    SLresult lastError();    // last error code

public:
    // 3 phase controlling methods: prepare, start, stop
    bool prepareRecording();

    bool startRecording();

    bool stopRecording();

    // get / set configurations for this recorder instance
    int setConfig(int configID, int value);

    int getConfig(int configID);

    // a java thread for use with jni. loop internal, exit until stopRecording called
    bool runEncoding(JNIEnv * env, jclass clzz, jobject javaObject);

    // called when voice data received from peer
    bool playDataAvailable(char* buffer, int len);

private:
    // ----------------------------------------------------------------------------------------------------
    // record & play callback, forward to object callback
    static void openslPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

    static void openslRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

    void playerCallback(SLAndroidSimpleBufferQueueItf bq);

    void recorderCallback(SLAndroidSimpleBufferQueueItf bq);

    bool decodeNextFrame(bool decodeMore);

    // ----------------------------------------------------------------------------------------------------

    // internal initialization
    bool createOpenSLEngine();

    bool createAudioPlayer();

    bool createAudioRecorder();

    bool destroyOpenSLEngine();

    void applyAudioLatency(float latency);

    SLAint64 currentTime();

    int calculateAudioLatency();

    void initBufferBoundry();
    void checkBufferBoundry(const char * const funcName, int line);

private:
    // ----------------------------------------------------------------------------------------------------
    bool mDebugMode;
    bool mDebugModeVerbose;
    bool mDebugLoopRecPlay;

    bool mInitialized;
    bool mPrepared;
    bool mRecording;

    SLresult mLastError;

    // status
    SLuint32 mSampleRate;
    int mSampleDepth;

    int mConfigRecordBufferSize;    // suggested recording buffer size from java, to keep minimal latency

    SLuint32 mConfigFlags;
    SLuint32 mConfigRecordMode;
    SLuint32 mConfigPlayStream;
    SLint32 mConfigRecPlayOffset; // samples count
    float mConfigRecordLevelPreProcess;

    // internal state
    bool mConfigMuteRecord;
    bool mConfigMutePlay;
    bool mConfigHandsFree;
    bool mConfigRecordLevelPreProcessEnabled;

    // calculated by sample rate & sample depth
    int mBytesPerFrame;
    int mSamplesPerFrame;

    NativeVoiceCodec* mVoiceCodec;

    // ----------------------------------------------------------------------------------------------------
    // engine interfaces

    SLObjectItf mEngineObject;
    SLEngineItf mEngineEngine;

    // ----------------------------------------------------------------------------------------------------
    // recorder interfaces

    SLObjectItf mRecorderObject;
    SLRecordItf mRecorderRecord;
    SLAndroidSimpleBufferQueueItf mRecorderBufferQueue;

    SLAuint64 mRecorderFrameCount;

    // continuous buffer of recorded audio at 8/16 kHz mono, 16-bit signed little endian
    void* mRecorderBufferBegin;
    short mRecorderBuffer[320 * (BUFFER_COUNT_MAX + 1)];
    void* mRecorderBufferEnd;
    int mRecorderSize;     // samples count for buffer
    SLAint64 mRecorderRead;     // total samples read
    SLAint64 mRecorderWrite;    // total samples write

    CSignal mSignal;

    // ----------------------------------------------------------------------------------------------------
    // player interfaces

    SLObjectItf mOutputMixObject;

    SLObjectItf mPlayerObject;
    SLPlayItf mPlayerPlay;
    SLAndroidSimpleBufferQueueItf mPlayerBufferQueue;
    SLVolumeItf mPlayerVolume;

    SLAuint64 mPlayerFrameCount;
    SLAuint64 mPlayerFakeFrameCount;
    SLAuint64 mSkippedFrameCount;

    // continuous buffer of playing audio at 8/16 kHz mono, 16-bit signed little endian
    void* mPlayerBufferBegin;
    short mPlayerBuffer[320 * (BUFFER_COUNT_MAX + 1)];
    void* mPlayerBufferEnd;
    int mPlayerSize;     // samples count for buffer
    SLAint64 mPlayerRead;     // total samples read for echo cancelling
    SLAint64 mPlayerWrite;    // total samples write to audio device
    SLAint64 mPlayerPending;  // next position to write pending samples for playing

    // fade in flags: 10 frames == 20ms * 10 = 200ms
    int mFadeInPlay;
    int mFadeInRecord;

    // ----------------------------------------------------------------------------------------------------

    // will not be affected by sample rate, no need to malloc dynamically
    struct PlayFrame mPendingBuffer[PENDING_FRAME_COUNT];
    int mPendingSize;
    SLAint64 mPendingRead;     // total frames read
    SLAint64 mPendingWrite;    // total frames write

    SLAint64 mInJitterRecovery; // frame count time stamp, read from 'mPendingRead' if jitter occurs

    // ----------------------------------------------------------------------------------------------------

    bool mRequestPlaybackData;
    bool mRequestPlaybackEvent;
    bool mRequestPlaybackNoTraffic;
    bool mRequestPlaybackTrafficOK;

    SLAuint64 mNoTrafficSince; // no traffic since 'mPlayerFrameCount'
    SLAuint64 mNoTrafficLastReport; // no traffic since last report

    int mNoTrafficFrameCount; // saved on traffic restore

    // ----------------------------------------------------------------------------------------------------

    latencyMeasurer* mLatencyMeasurer;
    IAcousticEchoCanceler* mEchoCanceler;

    float mKnowLatency;

    // ----------------------------------------------------------------------------------------------------
    // stable play/record monitoring

    SLAint64 mLastPlayPosForRecord;
    SLAint64 mLastRecordPosForPlay;

    int mStallLevelRecord;
    int mStallLevelPlay;

    SLAint64 mLastTimeForRecord;
    SLAint64 mLastTimeForPlay;

    int mFrameDurationPlay[STABLE_VOICE_COUNT];
    int mFrameDurationRecord[STABLE_VOICE_COUNT];

    int mFrameDurationPlayPos;
    int mFrameDurationRecordPos;

    int mCurrentStallLevel;
    SLint32 mAlignedRecordPlayOffset;

    int mPrevLatencyFromAEC;
    int mSameLatencyCount;

    SLuint32 mEstimatedLatency[LATENCY_SLOT]; // estimated device latency from aec module

    int mDeviceLatency;
    int mLatencyCount;

    SLAint64 mProducerConsumerAlignment;
    // ----------------------------------------------------------------------------------------------------
};

#endif //NATIVE_AUDIO_YEECALLRECORDER_H
