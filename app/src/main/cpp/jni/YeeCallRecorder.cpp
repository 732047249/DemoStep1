//
// Created by Qiongzhu Wan on 16-9-28.
//

#include <unistd.h>
#include <stdio.h>

#include "YeeCallRecorder.h"
#include "logcat.h"

#include "NativeVoiceCodec.h"
#include "latencyMeasurer.h"

// #define _DEBUG_MEMORY_BUFFER

#ifdef _DEBUG_MEMORY_BUFFER
#define CHECK_BOUNDRY \
        { checkBufferBoundry(__FUNCTION__, __LINE__); }
#else
#define CHECK_BOUNDRY (0)
#endif

//---------------------------------------------------------------------------------------------------------------------

int ESTIMATED_DELAY = 0;

//---------------------------------------------------------------------------------------------------------------------

// synthesize a mono sawtooth wave and place it into a buffer (called automatically on load)
__attribute__((constructor)) static void onDlOpen(void) {
    srand(time(NULL));
}

//---------------------------------------------------------------------------------------------------------------------
// event codes for java recorder

#define SL_EVENT_NO_TRAFFIC            1
#define SL_EVENT_TRAFFIC_OK            2
#define SL_EVENT_PLAYING_CONTINUES     3

//---------------------------------------------------------------------------------------------------------------------

/**
 * Throw exception with given class and message.
 *
 * @param env JNIEnv interface.
 * @param className class name.
 * @param message exception message.
 */
static void ThrowException(JNIEnv *env, const char *className, const char *message) {
    // Get the exception class
    jclass clazz = env->FindClass(className);

    // If exception class is found
    if (NULL != clazz) {
        // Throw exception
        env->ThrowNew(clazz, message);

        // Release local class reference
        env->DeleteLocalRef(clazz);
    }
}

/**
 * Convert OpenSL ES result to string.
 *
 * @param result result code.
 * @return result string.
 */
static const char *ResultToString(SLresult result) {
    const char *str = 0;

    switch (result) {
    case SL_RESULT_SUCCESS:
        str = "Success";
        break;
    case SL_RESULT_PRECONDITIONS_VIOLATED:
        str = "Preconditions violated";
        break;
    case SL_RESULT_PARAMETER_INVALID:
        str = "Parameter invalid";
        break;
    case SL_RESULT_MEMORY_FAILURE:
        str = "Memory failure";
        break;
    case SL_RESULT_RESOURCE_ERROR:
        str = "Resource error";
        break;
    case SL_RESULT_RESOURCE_LOST:
        str = "Resource lost";
        break;
    case SL_RESULT_IO_ERROR:
        str = "IO error";
        break;
    case SL_RESULT_BUFFER_INSUFFICIENT:
        str = "Buffer insufficient";
        break;
    case SL_RESULT_CONTENT_CORRUPTED:
        str = "Success";
        break;
    case SL_RESULT_CONTENT_UNSUPPORTED:
        str = "Content unsupported";
        break;
    case SL_RESULT_CONTENT_NOT_FOUND:
        str = "Content not found";
        break;
    case SL_RESULT_PERMISSION_DENIED:
        str = "Permission denied";
        break;
    case SL_RESULT_FEATURE_UNSUPPORTED:
        str = "Feature unsupported";
        break;
    case SL_RESULT_INTERNAL_ERROR:
        str = "Internal error";
        break;
    case SL_RESULT_UNKNOWN_ERROR:
        str = "Unknown error";
        break;
    case SL_RESULT_OPERATION_ABORTED:
        str = "Operation aborted";
        break;
    case SL_RESULT_CONTROL_LOST:
        str = "Control lost";
        break;
    default:
        str = "Unknown code";
    }

    return str;
}

char* dumpAudioSamples(short* memPtr, int sampleCount) {
    static char logBuffer[64 * 1024];

    int offset = 0;
    for (int i = 0; i < sampleCount; i++) {
        char* p = (char*)(memPtr + i);
        offset += sprintf(logBuffer + offset, "%02x%02x", *(p+1), *p);
    }
    logBuffer[offset] = 0; // ensure a zero byte is always written

    return logBuffer;
}

char* dumpMemory(char* memPtr, int bytesCount) {
    static char logBuffer[64 * 1024];

    int offset = 0;
    for (int i = 0; i < bytesCount; i++) {
        char* p = (char*)(memPtr + i);
        offset += sprintf(logBuffer + offset, "%02x", *p);
    }
    logBuffer[offset] = 0; // ensure a zero byte is always written

    return logBuffer;
}

#define RETURNIF_ERROR_VOID(exp) \
    { SLresult _result = (exp); if (SL_RESULT_SUCCESS != _result) { mLastError = _result; LOGI("OpenSL return failure: %d - %s, line:%d, func: %s", _result, ResultToString(_result), __LINE__, __FUNCTION__); return; } else { /* NOP */ }; }

#define RETURNIF_ERROR_BOOL(exp) \
    { SLresult _result = (exp); if (SL_RESULT_SUCCESS != _result) { mLastError = _result; LOGI("OpenSL return failure: %d - %s, line:%d, func: %s", _result, ResultToString(_result), __LINE__, __FUNCTION__); return false; } else { /* NOP */ }; }

#define WARNIF_ERROR(exp) \
    { SLresult _result = (exp); if (SL_RESULT_SUCCESS != _result) { mLastError = _result; LOGI("OpenSL warn failure: %d - %s, line:%d, func: %s", _result, ResultToString(_result), __LINE__, __FUNCTION__); } else { /* NOP */ }; }

//---------------------------------------------------------------------------------------------------------------------

YeeCallRecorder::YeeCallRecorder() : mInitialized(false), mPrepared(false), mRecording(false), mLastError(SL_RESULT_SUCCESS), mSignal("recorder") {
    mDebugMode = false;
    mDebugModeVerbose = false;
    mDebugLoopRecPlay = false;

    // status
    mSampleRate = 0;
    mSampleDepth = 0;

    mConfigRecordBufferSize = 0;

    mConfigFlags = 0;
    mConfigRecordMode = 0;
    mConfigPlayStream = 0;
    mConfigRecPlayOffset = 0;
    mConfigRecordLevelPreProcessEnabled = false;

    // internal state
    mConfigMuteRecord = true;
    mConfigMutePlay = false;
    mConfigHandsFree = false;
    mConfigRecordLevelPreProcess = 1.0f;

    // calculated by sample rate & sample depth
    mBytesPerFrame = 0;
    mSamplesPerFrame = 0;

    mVoiceCodec = NULL;
    mEchoCanceler = NULL;

    // engine interfaces
    mEngineObject = NULL;
    mEngineEngine = NULL;

    // output mix
    mOutputMixObject = NULL;

    // recorder interfaces
    mRecorderObject = NULL;
    mRecorderRecord = NULL;
    mRecorderBufferQueue = NULL;

    // player interfaces
    mPlayerObject = NULL;
    mPlayerPlay = NULL;
    mPlayerBufferQueue = NULL;
    mPlayerVolume = NULL;

    // recorded audio at 8/16 kHz mono, 16-bit signed little endian
    mRecorderFrameCount = 0 ;
    mRecorderSize = 0;
    mRecorderRead = 0;
    mRecorderWrite = 0;

    // continuous buffer of playing audio at 8/16 kHz mono, 16-bit signed little endian
    mPlayerFrameCount = 0;
    mPlayerFakeFrameCount = 0;
    mSkippedFrameCount = 20; // start with some skipped frames
    mPlayerSize = 0;     // samples count for buffer
    mPlayerRead = 0;     // total samples read
    mPlayerWrite = 0;    // total samples write
    mPlayerPending = 0;

    mPendingSize = PENDING_FRAME_COUNT;
    mPendingRead = 0;     // total samples read
    mPendingWrite = 0;    // total samples write
    mInJitterRecovery = -1;

    mRequestPlaybackData = false;
    mRequestPlaybackEvent = false;
    mRequestPlaybackNoTraffic = false;
    mRequestPlaybackTrafficOK = false;
    mNoTrafficSince = 0;
    mNoTrafficLastReport = 0;
    mNoTrafficFrameCount = 0;

    // fade in flags: 10 frames == 20ms * 10 = 200ms
    mFadeInPlay = 20;
    mFadeInRecord = 20;

    mLastPlayPosForRecord = 0;
    mLastRecordPosForPlay = 0;

    mLastTimeForRecord = 0;
    mLastTimeForPlay = 0;

    mStallLevelRecord = 0;
    mStallLevelPlay = 0;

    memset(mFrameDurationPlay, 0, sizeof(mFrameDurationPlay));
    memset(mFrameDurationRecord, 0, sizeof(mFrameDurationRecord));

    mFrameDurationPlayPos = 0;
    mFrameDurationRecordPos = 0;

    mCurrentStallLevel = 0;
    mAlignedRecordPlayOffset = 0;

    mPrevLatencyFromAEC = 0;
    mSameLatencyCount = 0;

    memset(mEstimatedLatency, 0, sizeof(mEstimatedLatency));
    mDeviceLatency = 0;
    mLatencyCount = 0;

    mProducerConsumerAlignment = -1;

    initBufferBoundry();
}

YeeCallRecorder::~YeeCallRecorder() {
    destroyOpenSLEngine();
}

//---------------------------------------------------------------------------------------------------------------------
// allocate opensl engine

bool YeeCallRecorder::create(int sampleRate, int recordBufferSize, int flags, int recordFlags, int playFlags, int recPlayOffsetMS) {
    // currently, only 8KHz / 16KHz sample rates are supported
    if (sampleRate != 8000 && sampleRate != 16000) {
        LOGI("sample rate not supported: %d", sampleRate);
        return false;
    }

    // only 8KHz / 16KHz
    mSampleRate = sampleRate;
    mSampleDepth = 2; // only 16bit sample depth is supported

    mSamplesPerFrame = mSampleRate * RECORDER_DURATION_MS / 1000;
    mBytesPerFrame = mSamplesPerFrame * mSampleDepth;

    mConfigRecordBufferSize = recordBufferSize;

    mConfigFlags = flags;
    mConfigRecordMode = recordFlags < 0 ? SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION : recordFlags;
    mConfigPlayStream = playFlags < 0 ? SL_ANDROID_STREAM_VOICE : playFlags;

    mConfigRecPlayOffset = recPlayOffsetMS > 0 ? recPlayOffsetMS * mSampleRate / 1000 : 0; // samples count, defaults to 100ms (5 frames)

    // now resolving requested flags
    mDebugMode = ((mConfigFlags & FLAGS_DEBUG_MODE) == FLAGS_DEBUG_MODE);
    mDebugModeVerbose = ((mConfigFlags & FLAGS_DEBUG_MODE_VERBOSE) == FLAGS_DEBUG_MODE_VERBOSE);
    mDebugLoopRecPlay = ((mConfigFlags & FLAGS_DEBUG_LOOP_REC_PLAY) == FLAGS_DEBUG_LOOP_REC_PLAY);

    mRequestPlaybackData = false;
    mRequestPlaybackEvent = false;
    mRequestPlaybackNoTraffic = false;
    mRequestPlaybackTrafficOK = false;
    mNoTrafficSince = 0;
    mNoTrafficLastReport = 0;
    mKnowLatency = -1.0f;

    LOGI("create YeeCallRecorder: %dHz %dbit with samplesPerFrame: %d, bytesPerFrame: %d, recordBuffer: %d, debug: %d, flags %d, recordMode: %d, playMode: %d, recPlayOffset: %d",
         mSampleRate, mSampleDepth * 8, mSamplesPerFrame, mBytesPerFrame, recordBufferSize, mDebugMode, mConfigFlags, mConfigRecordMode, mConfigPlayStream, mConfigRecPlayOffset);

    if (createOpenSLEngine()) {
        LOGI("create opensl engine success");
        return true;
    } else {
        LOGI("create opensl engine failed");
        return false;
    }
}

void YeeCallRecorder::destroy() {
    destroyOpenSLEngine();
}

SLresult YeeCallRecorder::lastError() {
    return mLastError;
}

//---------------------------------------------------------------------------------------------------------------------

void YeeCallRecorder::openslPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    YeeCallRecorder* recorder = (YeeCallRecorder*) context;
    recorder->playerCallback(bq);
}

// this callback handler is called every time a buffer finishes playing
void YeeCallRecorder::playerCallback(SLAndroidSimpleBufferQueueItf bq) {
    assert(bq == mPlayerBufferQueue);

    bool noData = false; // a local flag for fade in. not treated as lack of playback data

    // this callback running in opensl thread which could not access the jni env.
    // so, we can only generate a empty frame if lack of data

    // if no pending frame decoded, try to decode one now
    decodeNextFrame(false);

    // LOGI("player call back - total: %lld, R: %lld, W: %lld, P: %lld, size: %d, ", mPlayerFrameCount, mPlayerRead, mPlayerWrite, mPlayerPending, mPlayerSize);

    if (mPlayerPending - mPlayerWrite >= mSamplesPerFrame) {
        if (!mRequestPlaybackTrafficOK && mNoTrafficSince > 0) {
            mRequestPlaybackTrafficOK = true;
            mNoTrafficFrameCount = mPlayerFrameCount - mNoTrafficSince;
            LOGI("playback data restored. missing: %d, %lld", mNoTrafficFrameCount, mNoTrafficSince);
        }

        // has playing data now
        mRequestPlaybackData = false;
        mNoTrafficSince = 0;
        mNoTrafficLastReport = 0;
    }

    // no pending pages found. send a empty frame to keep player running
    while (mPlayerPending - mPlayerWrite < mSamplesPerFrame) {
        // allocate another & enqueue
        int pos = mPlayerPending % mPlayerSize;
        memset(mPlayerBuffer + pos, 0, mBytesPerFrame);
        mPlayerPending += mSamplesPerFrame;

        noData = true;

        // LOGI("insert empty frames: %llu, %llu, %lld", mPlayerFrameCount, mPlayerFakeFrameCount, mInJitterRecovery);

        if (mSkippedFrameCount > 0) {
            mSkippedFrameCount--; // skipped pure silence, restore silence. treat as if we have data
            // LOGI("restore skipped frame on no data: %lld",  mSkippedFrameCount);
        } else {
            mPlayerFakeFrameCount++;
            mRequestPlaybackData = true; // could not invoke java method from OpenSL thread, set the flag and request in encode
            mInJitterRecovery = mPendingRead;

            // after voice fully startup
            if (mNoTrafficSince == 0 && mPlayerFrameCount > 100 && mPlayerFrameCount > mPlayerFakeFrameCount) {
                mNoTrafficSince = mPlayerFrameCount;
                mNoTrafficLastReport = mPlayerFrameCount;
                LOGI("begin no data to play: %lld", mNoTrafficSince);
            }
        }

        // a fade in is required
        if (mFadeInPlay < 10) {
            mFadeInPlay = 10; // set to 50%
        }

        if (false && mDebugModeVerbose) {
            if (mSkippedFrameCount <= 0) {
                LOGI("no voice playing data, an empty frame inserted: %lld", mSkippedFrameCount);
            } else {
                LOGI("silence restored for skipped frame: %lld", mSkippedFrameCount);
            }
        }
    }

    // now we have the data for playback
    if (mPlayerPending - mPlayerWrite >= mSamplesPerFrame) {
        // OK, we have pending frames to play
        mPlayerFrameCount++;

        // LOGI("pending play frame found. continue: %d", mPlayerFrameCount);

        // allocate another & enqueue
        int pos = mPlayerWrite % mPlayerSize;
        mPlayerWrite += mSamplesPerFrame;
        int newPos = mPlayerWrite % mPlayerSize;


        if (mFadeInPlay > 0 && !noData) {
            // perform fade in. 0 --> 100%, 50 --> 0%
            const float scale = 1 - mFadeInPlay / 50.0f;
            if (mDebugModeVerbose) {
                LOGI("fade in: %d, %03f", mFadeInPlay, scale);
            }

            mFadeInPlay--;

            // TODO: a more precise fade in method is necessary
            if (scale > 0) {
                for (int i = 0; i < mSamplesPerFrame; i++) {
                    mPlayerBuffer[pos + i] = (short) (mPlayerBuffer[pos + i]  * scale);
                }
            }
        }

        if (mConfigMutePlay) {
            // discard recorded data for mute mic
            memset(mPlayerBuffer + pos, 0, mBytesPerFrame);
        }

        // right before playback: generate latency measuring signal if necessary
        latencyMeasurer* measurer = mLatencyMeasurer;
        if (measurer) {
            // check if measure completion
            if (measurer->mCurrentMeasurementState != enumPassthrough) {
                // generate output at 'pos'
                measurer->processOutput(mPlayerBuffer + pos, mSampleRate, mSamplesPerFrame);
                // LOGI("generate samples - state: %d, %s", measurer->mState,  dumpAudioSamples(mRecorderBuffer + pos, mSamplesPerFrame));
            } else {
                mLatencyMeasurer = NULL; // remove

                LOGI("latency measure completed with result: %d", measurer->mLatencyMs);
                applyAudioLatency(measurer->mLatencyMs);

                delete measurer;

                // mute for some time, and do forget current
                mFadeInPlay = 20;
                mFadeInRecord = 20;
                memset(mPlayerBuffer + pos, 0, mBytesPerFrame);
            }
        }

        // LOGI("now playing the pending frame: %lld, pos: %d", mPlayerWrite, pos);
        WARNIF_ERROR((*mPlayerBufferQueue)->Enqueue(mPlayerBufferQueue, mPlayerBuffer + pos, mBytesPerFrame));

        if (mPlayerWrite % (50 * mSamplesPerFrame) == 0) {
            int pos = ESTIMATED_DELAY >> 2;
            if (ESTIMATED_DELAY >= 0 && pos < LATENCY_SLOT && pos >= 0) {
                // save estimated delay every seconds
                mEstimatedLatency[pos]++;

                if (ESTIMATED_DELAY != mPrevLatencyFromAEC) {
                    mPrevLatencyFromAEC = ESTIMATED_DELAY;
                    mSameLatencyCount = 1;
                } else {
                    mSameLatencyCount++;
                }

                if (mConfigRecPlayOffset > 0 && ESTIMATED_DELAY <= 4 && mSameLatencyCount > 5) {
                    LOGI("estimated latency out of range. reset & ignore: %d", ESTIMATED_DELAY);
                    mDeviceLatency = 0;
                    mLatencyCount = 0;
                    mConfigRecPlayOffset = 0;
                }

                // print latency statistics every seconds
                if (mDebugModeVerbose) {
                    for (int i = 0; i < 125; i++) {
                        SLuint32 count = mEstimatedLatency[i];
                        if (count > 0) {
                            LOGI("delay: %d ms - %d", i * 4, count);
                        }
                    }
                }
            }
        }

        // mark as play done. save time stamp & analysis
        SLAint64 now = currentTime();
        SLAint64 previous = mLastTimeForPlay;
        SLAint64 duration = now - previous;

        mFrameDurationPlay[mFrameDurationPlayPos % STABLE_VOICE_COUNT] = duration;
        mFrameDurationPlayPos++;

        // check if recording continues while playing
        if (mLastRecordPosForPlay >= mRecorderWrite) {
            // stalled
            mStallLevelRecord++;
        } else {
            // recording continues
            mStallLevelRecord = 0;
        }
        mLastRecordPosForPlay = mRecorderWrite;

        if (mStallLevelRecord > STALL_LEVEL_LIMIT) {
            mCurrentStallLevel = STALL_RECOVERY;

            if (false) {
                SLAint64 totalDuration = 0;
                SLAint64 minDuration = 10000000L;
                SLAint64 maxDuration = -1;
                for (int i = 0; i < STABLE_VOICE_COUNT; i++) {
                    SLAint64 current = mFrameDurationPlay[i];
                    if (current <minDuration) {
                        minDuration = current;
                    }
                    if (current > maxDuration) {
                        maxDuration = current;
                    }
                    totalDuration += current;
                }

                LOGI("last play frame: current = %lld, min = %lld, max = %lld, avg = %f, record stall: %d",
                     duration, minDuration, maxDuration, (totalDuration * 1.0f / STABLE_VOICE_COUNT), mStallLevelRecord);
            }
        }

        // finally
        mLastTimeForPlay = now;
    }

    // after enqueue, decode next frame in advance
    decodeNextFrame(false);

    if (mPlayerFrameCount % SEND_PLAYBACK_EVENTS_FRAME_COUNT == 0) {
        mRequestPlaybackEvent = true;
    }

    if (mNoTrafficSince != 0) {
        if (!mRequestPlaybackNoTraffic && (mPlayerFrameCount - mNoTrafficLastReport > SEND_PLAYBACK_NO_TRAFFIC_FRAME_COUNT)) {
            mRequestPlaybackNoTraffic = true;
            mNoTrafficLastReport = mPlayerFrameCount;
        }
    }

    if (mRequestPlaybackData || mRequestPlaybackEvent || mRequestPlaybackNoTraffic || mRequestPlaybackTrafficOK) {
        if (false && !mRequestPlaybackData) {
            LOGI("generate playback event: %lld - reqData:%d, reqEvent:%d, reqNoTraffic:%d, reqTrafficOK:%d",
                 mPlayerFrameCount, mRequestPlaybackData, mRequestPlaybackEvent, mRequestPlaybackNoTraffic, mRequestPlaybackTrafficOK);
        }

        // send signal to request more data from java
        mSignal.notify();
    }

    if (mDebugMode) {
        if (mPlayerWrite % (100 * mSamplesPerFrame) == 0) {
            LOGI("playerCallback: playerWrite=%lld, play queueLen=%lld, latency from aec = %d ms, rec/play offset = %d ms, device latency = %d ms, producerDiff=%lld, consumerDiff=%lld, diff=%lld"
                 , mPlayerWrite, (mPendingWrite - mPendingRead), ESTIMATED_DELAY, (mConfigRecPlayOffset * 1000 / mSampleRate), mDeviceLatency, (mRecorderWrite - mPlayerWrite), (mRecorderRead - mPlayerRead), (mRecorderWrite - mPlayerWrite) - (mRecorderRead - mPlayerRead));
        }
    }
}

void YeeCallRecorder::openslRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    YeeCallRecorder* recorder = (YeeCallRecorder*) context;
    recorder->recorderCallback(bq);
}

// this callback handler is called every time a buffer finishes recording
void YeeCallRecorder::recorderCallback(SLAndroidSimpleBufferQueueItf bq) {
    assert(bq == mRecorderBufferQueue);

    mRecorderFrameCount++;

    // LOGI("recorder call back - total: %d, R: %d, W: %d, size: %d", mRecorderFrameCount, mRecorderRead, mRecorderWrite, mRecorderSize);
    latencyMeasurer* measurer = mLatencyMeasurer;

    // 1. allocate another & enqueue
    int pos = mRecorderWrite % mRecorderSize;

    if ((mConfigMuteRecord && measurer == NULL) || mFadeInRecord > 0) {
        // discard recorded data for mute mic, delay clear for latency measuring
        memset(mRecorderBuffer + pos, 0, mBytesPerFrame);

        if (mFadeInRecord > 0) {
            mFadeInRecord--;
        }
    }

    mRecorderWrite +=  mSamplesPerFrame;
    int newPos = mRecorderWrite % mRecorderSize;

    // 2. check if there is enough buffer to run recording
    while (mRecorderRead - mRecorderWrite > mRecorderSize - mSamplesPerFrame) {
        if (mDebugModeVerbose) {
            LOGI("no enough buffer to run recording. some thing must go wrong. now drop a audio frame. recorderRead: %lld, recorderWrite: %lld", mRecorderRead, mRecorderWrite);
        }

        mRecorderRead += mSamplesPerFrame;
    }

    // will write to the new position, old position is now available for encode
    RETURNIF_ERROR_VOID((*bq)->Enqueue(bq, mRecorderBuffer + newPos, mBytesPerFrame));

    // mark as play done. save time stamp & analysis
    SLAint64 now = currentTime();
    SLAint64 previous = mLastTimeForRecord;
    SLAint64 duration = now - previous;

    mFrameDurationRecord[mFrameDurationRecordPos % STABLE_VOICE_COUNT] = duration;
    mFrameDurationRecordPos++;

    // check if recording continues while playing
    if (mLastPlayPosForRecord >= mPlayerWrite) {
        // stalled
        mStallLevelPlay++;
    } else {
        // playing continues
        mStallLevelPlay = 0;
    }
    mLastPlayPosForRecord = mPlayerWrite;

    if (mStallLevelPlay > STALL_LEVEL_LIMIT) {
        mCurrentStallLevel = STALL_RECOVERY;

        if (false) {
            SLAint64 totalDuration = 0;
            SLAint64 minDuration = 10000000L;
            SLAint64 maxDuration = -1;
            for (int i = 0; i < STABLE_VOICE_COUNT; i++) {
                SLAint64 current = mFrameDurationRecord[i];
                if (current < minDuration) {
                    minDuration = current;
                }
                if (current > maxDuration) {
                    maxDuration = current;
                }
                totalDuration += current;
            }

            LOGI("last record frame: current = %lld, min = %lld, max = %lld, avg = %f, play stall: %d",
                 duration, minDuration, maxDuration, (totalDuration * 1.0f / STABLE_VOICE_COUNT), mStallLevelPlay);
        }
    }

    // finally
    mLastTimeForRecord = now;

    // LOGI("recorded: %d --> %d, total: %d - %s", pos, newPos, (newPos - pos), dumpAudioSamples(mRecorderBuffer + pos, mSamplesPerFrame));
    // LOGI("recorded: %d --> %d, total: %d", pos, newPos, (newPos - pos));

    if (measurer) {
        // check if measure completion
        if (measurer->mCurrentMeasurementState != enumPassthrough) {
            measurer->processInput(mRecorderBuffer + pos, mSampleRate, mSamplesPerFrame);
        } else {
            mLatencyMeasurer = NULL; // remove

            LOGI("latency measure completed with result: %d", measurer->mLatencyMs);
            applyAudioLatency(measurer->mLatencyMs);

            delete measurer;

            // mute for some time, and do forget current
            mFadeInPlay = 20;
            mFadeInRecord = 20;
        }

        // measurer is active, mute the voice from peer
        memset(mRecorderBuffer + pos, 0, mBytesPerFrame);
    }

    // 3. send signal to process the previous data
    mSignal.notify();
}

bool YeeCallRecorder::decodeNextFrame(bool decodeMore) {
    // decode and append to player pending buffer
    while (mPlayerPending - mPlayerWrite < mSamplesPerFrame) {
        if (mPendingRead < mPendingWrite) {
            const int posRead = mPendingRead % PENDING_FRAME_COUNT;
            mPendingRead++;
            const int posReadNext = mPendingRead % PENDING_FRAME_COUNT;

            PlayFrame* frame = mPendingBuffer + posRead;
            PlayFrame* frameNext = (mPendingWrite > mPendingRead ) ? mPendingBuffer + posReadNext : NULL;

            // write to here
            const int posPending = mPlayerPending % mPlayerSize;
            bool resampled = false;

            // do decode
            NativeVoiceCodec* ptrCodec = mVoiceCodec;
            if (ptrCodec) {
                // int decode(jbyte* encodedData, int encodedLen, jshort* decodedData, int maxDecodeLen, int decodeFEC);
                if (frame->length > 0) {
                    // decode normally
                    mRequestPlaybackData = false; // a normal frame decoded, no need to request data now
                    ptrCodec->decode(frame->data, frame->length, mPlayerBuffer + posPending, mBytesPerFrame, 0);
                } else {
                    if (frameNext && frameNext->length > 0) {
                        // LOGI("frame lost, a fec frame decoed");
                        // bad current frame: decode with fec
                        ptrCodec->decode(frameNext->data, frameNext->length, mPlayerBuffer + posPending, mBytesPerFrame, 1);
                    } else {
                        // LOGI("frame lost, a fec frame is not available");
                        // bad current frame: fec information not available
                        ptrCodec->decode(NULL, 0, mPlayerBuffer + posPending, mBytesPerFrame, 1);
                    }
                }

                // pre-sampling to simulate lack of data. keep it as false for production code
                if (false) {
                    const short* ptrStart = mPlayerBuffer + posPending;
                    const short* ptrEnd = ptrStart + mSamplesPerFrame;

                    const int reduceLevel = 16;
                    const int targetLevel = reduceLevel - 1;

                    short* ptrToWrite = mPlayerBuffer + posPending;
                    for (int i = 0; i < mSamplesPerFrame; i++) {
                        short sample = *(ptrStart + i);

                        if (i % reduceLevel < targetLevel) {
                            *ptrToWrite = sample;
                            ptrToWrite++;
                        } else {
                            // average the last sample
                            *(ptrToWrite - 1) = (short) ((int)(*(ptrToWrite - 1)) + (int)(sample)) / 2;
                        }
                    }

                    // mark decode complete, available for read
                    mPlayerPending += (ptrToWrite - ptrStart);
                    resampled = true;

                    // LOGI("speed up with simple resample: totalSamples=%d, reducedTo=%d", mSamplesPerFrame, (ptrToWrite - ptrStart));
                }
            } else {
                memset(mPlayerBuffer + posPending, 0, mBytesPerFrame); // clear the buffer
            }

            // ---------------------------------------------------------------------------------------------
            // resolve post decode actions: skip or rewind

            int actions = POST_DECODE_ACTION_NONE;

            if (mInJitterRecovery <= 0) {
                // jitter recovery mode is inactive

                if (mPendingRead + 2 <= mPendingWrite) {
                    // after decode, check if continuous silence for safe skip
                    if (frameNext) {
                        if (frame->length <= 1 && frameNext->length <= 1) {
                            // skip one to lower down playback latency if both are silence frames
                            actions = POST_DECODE_ACTION_SKIP_SILENCE;
                        }
                    } else {
                        // no next present? should not happen here
                    }
                }

                if (actions < 0 && mPendingRead + 4 >= mPendingWrite) {
                    // after decode, check if silence for safe add
                    if (frame->length <= 1) {
                        // current is slience: insert more silence to avoid voice jitter if both are silence frames
                        actions = POST_DECODE_ACTION_REWIND_SILENCE;
                    }
                }

                if (actions < 0 && mPendingRead + 40 < mPendingWrite) {
                    // too many pending audio to play, drop or resample

                    int percent = rand() * 1000.0f / RAND_MAX;
                    // LOGI("check random: %d, MAX: %d", percent, RAND_MAX);

                    int diff = mPendingWrite - mPendingRead;

                    if (diff > 80) {
                        actions = POST_DECODE_ACTION_RESAMPLE_SPEEDUP4;
                    } else if (60 < diff && diff <= 80) {
                        actions = POST_DECODE_ACTION_RESAMPLE_SPEEDUP3;
                    } else if (50 < diff && diff <= 60) {
                        actions = POST_DECODE_ACTION_RESAMPLE_SPEEDUP2;
                    } else if (40 < diff && diff <= 50) {
                        actions = POST_DECODE_ACTION_RESAMPLE_SPEEDUP1;
                    }

                    //    // check if too many frames, drop to catch up. > 0.8s
                    //    if (diff > 80) {
                    //        // drop 50% if total > 80
                    //        actions = POST_DECODE_ACTION_SKIP_ANY;
                    //    } else if (60 < diff && diff <= 80) {
                    //        if (percent > 800) {
                    //            // drop 10% if total > 60
                    //            actions = POST_DECODE_ACTION_SKIP_ANY;
                    //        }
                    //    } else if (50 < diff && diff <= 60) {
                    //        if (percent > 900) {
                    //            // drop 5% if total > 50
                    //            actions = POST_DECODE_ACTION_SKIP_ANY;
                    //        }
                    //    } else if (40 < diff && diff <= 50) {
                    //        if (percent > 960) {
                    //            // drop 2% if total > 40
                    //            actions = POST_DECODE_ACTION_SKIP_ANY;
                    //        }
                    //    }

                    // if (skipFrame) {
                    //     LOGI("skip frame by random: %d, diff: %lld", percent, (mPendingWrite - mPendingRead));
                    // }
                }
            } else {
                // jitter recovery mode is active
                if (mPendingRead + 40 < mPendingWrite) {
                    // if too many delay created during jitter recovery, stop the mode
                    LOGI("too many delay created during jitter recovery. stop now: %lld, %lld, %d", mPendingRead, mInJitterRecovery, (int)(mPendingWrite - mPendingRead));
                    mInJitterRecovery = -1;
                }

                if (mPendingRead - mInJitterRecovery > JITTER_RECOVERY_COUNT) {
                    // if 2 seconds elapsed since last jitter recovery, stop the mode
                    LOGI("no jitter found during last recovery. stop now: %lld, %lld, %d", mPendingRead, mInJitterRecovery, (int)(mPendingWrite - mPendingRead));
                    mInJitterRecovery = -1;
                }

                if (mInJitterRecovery > 0 && frame->length <= 1 && mPendingRead + 25 > mPendingWrite) {
                    // current is slience: insert more silence to avoid voice jitter if both are silence frames
                    // LOGI("rewind silence and wait for more data: %lld", (mPendingWrite - mPendingRead));
                    actions = POST_DECODE_ACTION_REWIND_SILENCE;
                }
            }

            // ---------------------------------------------------------------------------------------------
            // lower down the latency by drop some unimportant frames, or resample to speedup playback

            switch (actions) {
            case POST_DECODE_ACTION_SKIP_SILENCE:
            case POST_DECODE_ACTION_SKIP_ANY:
            case POST_DECODE_ACTION_FAST_FORWARD: {
                mPendingRead++; // do skip
                if (mSkippedFrameCount < 50) {
                    mSkippedFrameCount++; // add statical counter
                }
                if (mDebugModeVerbose) {
                    LOGI("safe skip %lld for latency, total skipped: %lld, playQueue: %lld", mPendingRead, mSkippedFrameCount, (mPendingWrite - mPendingRead));
                }
                break;
            }
            case POST_DECODE_ACTION_REWIND_SILENCE: {
                if (mSkippedFrameCount > 0) {
                    mPendingRead--; // do rewind
                    mSkippedFrameCount--; // substract statical counter. do balanced rewind
                    if (mDebugModeVerbose) {
                        LOGI("safe rewind %lld for latency, total skipped: %lld, playQueue: %lld", mPendingRead, mSkippedFrameCount, (mPendingWrite - mPendingRead));
                    }
                }
                break;
            }
            case POST_DECODE_ACTION_RESAMPLE_SPEEDUP1:
            case POST_DECODE_ACTION_RESAMPLE_SPEEDUP2:
            case POST_DECODE_ACTION_RESAMPLE_SPEEDUP3:
            case POST_DECODE_ACTION_RESAMPLE_SPEEDUP4: {
                if (resampled) {
                    break;
                }

                // speed up the playback by sampling the data
                const short* ptrStart = mPlayerBuffer + posPending;
                const short* ptrEnd = ptrStart + mSamplesPerFrame;

                int reduceLevel = 16; // defaults

                switch (actions) {
                case POST_DECODE_ACTION_RESAMPLE_SPEEDUP1:
                    reduceLevel = 32;
                    break;
                case POST_DECODE_ACTION_RESAMPLE_SPEEDUP2:
                    reduceLevel = 16;
                    break;
                case POST_DECODE_ACTION_RESAMPLE_SPEEDUP3:
                    reduceLevel = 8;
                    break;
                case POST_DECODE_ACTION_RESAMPLE_SPEEDUP4:
                    reduceLevel = 4;
                    break;
                }

                int targetLevel = reduceLevel - 1;

                short* ptrToWrite = mPlayerBuffer + posPending;
                for (int i = 0; i < mSamplesPerFrame; i++) {
                    short sample = *(ptrStart + i);

                    if (i % reduceLevel < targetLevel) {
                        *ptrToWrite = sample;
                        ptrToWrite++;
                    } else {
                        // average the last sample
                        *(ptrToWrite - 1) = (short) ((int)(*(ptrToWrite - 1)) + (int)(sample)) / 2;
                    }
                }

                // mark decode complete, available for read
                mPlayerPending += (ptrToWrite - ptrStart);
                resampled = true;

                // LOGI("speed up with simple resample: totalSamples=%d, reducedTo=%d", mSamplesPerFrame, (ptrToWrite - ptrStart));
                break;
            }
            }

            if (!resampled) {
                // mark decode complete, available for read
                mPlayerPending += mSamplesPerFrame;
            }

            // drop all if latency too large. should less than PENDING_FRAME_COUNT to avoid buffer overflow
            if (mPendingWrite - mPendingRead > 100) {
                // TODO: search for the last continuous silence to cut off
                LOGI("too many pending frames for playback. drop all: %lld", (mPendingWrite - mPendingRead));
                mPendingRead = mPendingWrite;
                if (mFadeInPlay < 40) {
                    mFadeInPlay = 40;
                }
            }

            // ---------------------------------------------------------------------------------------------

            // do not return, loop check again if enough data for playback
            // return true;
        } else {
            // no pending frames to play, ignore
            break;
        }
    }

    if (mPlayerPending - mPlayerWrite >= mSamplesPerFrame) {
        // has enough pending data for playback, clear request
        mRequestPlaybackData = false;
    }

    // return true if decode make player happy, false if no data to play
    return (mPlayerPending - mPlayerWrite >= mSamplesPerFrame);
}

//---------------------------------------------------------------------------------------------------------------------

bool YeeCallRecorder::prepareRecording() {
    if (!createAudioRecorder()) {
        mRecording = false;
        mPrepared = false;
        return false;
    }

    if (!createAudioPlayer()) {
        mRecording = false;
        mPrepared = false;
        return false;
    }

    mPrepared = true;
    LOGI("prepare record/play success: %d", mPrepared);

    return true;
}

// set the recording state for the audio recorder
bool YeeCallRecorder::startRecording() {
    if (!mPrepared) {
        prepareRecording();
    }

    if (!mPrepared) {
        LOGE("Error: the recorder object is prepared with failure. could not start recording");
        return false;
    }

    // in case already recording, stop recording and clear buffer queue
    RETURNIF_ERROR_BOOL((*mRecorderRecord)->SetRecordState(mRecorderRecord, SL_RECORDSTATE_STOPPED));
    RETURNIF_ERROR_BOOL((*mRecorderBufferQueue)->Clear(mRecorderBufferQueue));

    RETURNIF_ERROR_BOOL((*mPlayerPlay)->SetPlayState(mPlayerPlay, SL_PLAYSTATE_STOPPED));
    RETURNIF_ERROR_BOOL((*mPlayerBufferQueue)->Clear(mPlayerBufferQueue));

    // enqueue recorder buffer to the buffer queue now
    mRecorderWrite = 0;
    RETURNIF_ERROR_BOOL((*mRecorderBufferQueue)->Enqueue(mRecorderBufferQueue, mRecorderBuffer, mBytesPerFrame));

    // enqueue zero to startup playing
    mPlayerRead = 0;
    memset(mPlayerBuffer, 0, mPlayerSize * sizeof(short)); // fill with zero, ensure the playing start with silence
    RETURNIF_ERROR_BOOL((*mPlayerBufferQueue)->Enqueue(mPlayerBufferQueue, mPlayerBuffer, mBytesPerFrame));

    // start recording & playing
    RETURNIF_ERROR_BOOL((*mRecorderRecord)->SetRecordState(mRecorderRecord, SL_RECORDSTATE_RECORDING));
    RETURNIF_ERROR_BOOL((*mPlayerPlay)->SetPlayState(mPlayerPlay, SL_PLAYSTATE_PLAYING));

    mFadeInPlay = 50; // perform a cold start
    mFadeInRecord = 50;

    mRecording = true;

    mLastTimeForPlay = mLastTimeForRecord = currentTime();

    LOGI("now record started");
    return true;
}

bool YeeCallRecorder::stopRecording() {
    if (!mRecording) {
        LOGI("not in recording state, ignore");
        return false;
    }

    if (mConfigHandsFree) {
        // calculate latency based on previous session: hands free --> earpiece
        int prevLatency = mDeviceLatency;
        int latency = calculateAudioLatency();
        if (latency >= 0) {
            if (mLatencyCount < 3) {
                mDeviceLatency = (mDeviceLatency * mLatencyCount + latency) / (mLatencyCount + 1);
            } else {
                mDeviceLatency = (mDeviceLatency * 3 + latency) / 4;
            }
            mLatencyCount++;

            LOGI("final latency: prev = %d, current = %d, final = %d", prevLatency, latency, mDeviceLatency);
        }
    }

    LOGI("now stop recording, fake play frames: %lld, total play frames: %lld, jitter: %03f%%",
         mPlayerFakeFrameCount, mPlayerFrameCount, (mPlayerFakeFrameCount * 100.0f / mPlayerFrameCount));

    mRecording = false;

    RETURNIF_ERROR_BOOL((*mRecorderRecord)->SetRecordState(mRecorderRecord, SL_RECORDSTATE_STOPPED));
    RETURNIF_ERROR_BOOL((*mRecorderBufferQueue)->Clear(mRecorderBufferQueue));

    RETURNIF_ERROR_BOOL((*mPlayerPlay)->SetPlayState(mPlayerPlay, SL_PLAYSTATE_STOPPED));
    RETURNIF_ERROR_BOOL((*mPlayerBufferQueue)->Clear(mPlayerBufferQueue));

    mSignal.notify();

    return true;
}

// get / set configurations for this recorder instance
int YeeCallRecorder::setConfig(int configID, int value) {
    const bool enable = value == 1; // 1 == enable, 0 == disable

    switch (configID) {
    case CFG_ACTION_MUTE_RECORD: {
        mConfigMuteRecord = enable;
        break;
    }
    case CFG_ACTION_MUTE_PLAY: {
        mConfigMutePlay = enable;
        break;
    }
    case CFG_ACTION_MUTE_ALL: {
        mConfigMuteRecord = enable;
        mConfigMutePlay = enable;
        break;
    }
    case CFG_ACTION_HANDS_FREE: {
        if (!enable && mConfigHandsFree) {
            // calculate latency based on previous session: hands free --> earpiece
            int prevLatency = mDeviceLatency;
            int latency = calculateAudioLatency();
            if (latency >= 0) {
                if (mLatencyCount < 3) {
                    mDeviceLatency = (mDeviceLatency * mLatencyCount + latency) / (mLatencyCount + 1);
                } else {
                    mDeviceLatency = (mDeviceLatency * 3 + latency) / 4;
                }
                mLatencyCount++;

                LOGI("latency: prev = %d, current = %d, final = %d", prevLatency, latency, mDeviceLatency);
            }
        }

        // reset estimated latency array
        memset(mEstimatedLatency, 0, sizeof(mEstimatedLatency));

        mConfigHandsFree = enable;

        // after hands free mode applied, echo canceler must be reseted
        NativeVoiceCodec* ptrCodec = mVoiceCodec;
        if (ptrCodec && mConfigHandsFree) {
            return ptrCodec->setCodecState(TYPE_AEC, AEC_TYPE_RESET);
        }

        break;
    }
    case CFG_ACTION_NOISE_DETECT: {
        NativeVoiceCodec* ptrCodec = mVoiceCodec;
        if (ptrCodec) {
           return ptrCodec->setCodecState(TYPE_NOISE_DETECT, value);
        }
        break;
    }
    case CFG_ACTION_DENOISE_MODE: {
        NativeVoiceCodec* ptrCodec = mVoiceCodec;
        if (ptrCodec) {
           return ptrCodec->setCodecState(TYPE_DENOISE_MODE, value);
        }
        break;
    }
    case CFG_VALUE_RECORDED:
    case CFG_VALUE_PLAYED:
    case CFG_VALUE_JITTER:
    case CFG_VALUE_PLAY_QUEUE: {
        LOGE("error: cfg=%d could not be updated, it's read only and informational", configID);
        break;
    }
    case CFG_VALUE_RECORD_PLAY_OFFSET: {
        // if (value > 0) {
        //     LOGI("record / play offset is now controlled automaticlly, data will not updated: %d", value);
        // }

        // ranges: 0 ~ 2 seconds
        if (value >= 0 && value <= 2000) {
            SLuint32 oldOffset = mConfigRecPlayOffset;
            mConfigRecPlayOffset = value * mSampleRate / 1000;
            LOGI("set rec / play offset: %d --> %d frames, time: %d ms", oldOffset, mConfigRecPlayOffset, value);
        }

        break;
    }
    case CFG_VALUE_RECORD_PRE_PROCESS: {
        float floatValue = *((float*)&value);

        // accept range: [0.01, 5)
        if (floatValue < 0.01 || floatValue > 5) {
            LOGI("bad pre process scale range: %03f, ignored", floatValue);
            mConfigRecordLevelPreProcessEnabled = false;
            mConfigRecordLevelPreProcess = 1.0f;
            break;
        }

        if ((floatValue - 1) >= 0.001f || (floatValue - 1) <= -0.001f) {
            mConfigRecordLevelPreProcessEnabled = true;
            mConfigRecordLevelPreProcess = floatValue;
        } else {
            mConfigRecordLevelPreProcessEnabled = false;
            mConfigRecordLevelPreProcess = 1.0f;
        }

        LOGI("apply pre process for recording: %3f, enabled: %d", mConfigRecordLevelPreProcess, mConfigRecordLevelPreProcessEnabled);

        break;
    }
    case CFG_ACTION_MEASURE_LATENCY: {
        latencyMeasurer* measurer = mLatencyMeasurer;
        if (mLatencyMeasurer || measurer) {
            LOGI("another measureing is in progress. abort current action");
            break;
        }

        measurer = new latencyMeasurer();
        // start measuring after latencyMeasurer installed
        measurer->startMeasuring(value);

        mLatencyMeasurer = measurer;

        LOGI("start latency measuring...");
        break;
    }
    case CFG_VALUE_SAMPLES_PER_FRAME: {
        LOGE("error: cfg=%d could not be updated, it's read only and informational", configID);
        break;
    }
    case CFG_VALUE_DEVICE_LATENCY: {
        // if previous value > 450, ignore and re-calibrate latency
        // if low latency. let webrtc handle it
        if (value > 60 && value < 450) {
            LOGI("update device latency: %d --> %d, c=%d", mLatencyCount, value, mLatencyCount);

            mDeviceLatency = value;
            if (mLatencyCount < 3) {
                mLatencyCount = 3;
            }

            // long latency. let me help webrtc for fast startup
            int targetOffset = value - 40;

            SLuint32 oldOffset = mConfigRecPlayOffset;
            mConfigRecPlayOffset = targetOffset * mSampleRate / 1000;

            LOGI("set rec / play offset automatically for aec fast startup: %d --> %d frames, time: %d ms", oldOffset, mConfigRecPlayOffset, targetOffset);
        } else {
            LOGI("reset device latency to all zero. supplied value = %d", value);

            mDeviceLatency = 0;
            mLatencyCount = 0;
            mConfigRecPlayOffset = 0;
        }
        break;
    }
    case CFG_VALUE_LIVE_LATENCY: {
        LOGE("error: cfg=%d could not be updated, it's read only and informational", configID);
        break;
    }

    default: {
        // forward to low level codec
        NativeVoiceCodec* ptrCodec = mVoiceCodec;
        if (ptrCodec) {
            return ptrCodec->setCodecState(configID, value);
        }
    }
    }

    return 0;
}

int YeeCallRecorder::getConfig(int configID) {
    if (configID & 0xf0000) {
        switch (configID) {
        case CFG_VALUE_RECORDED: {
            return (int) mRecorderFrameCount;
        }
        case CFG_VALUE_PLAYED: {
            return (int) mPlayerFrameCount;
        }
        case CFG_VALUE_JITTER: {
            return (int) mPlayerFakeFrameCount;
        }
        case CFG_VALUE_PLAY_QUEUE : {
            return (int) (mPendingWrite - mPendingRead);
        }
        case CFG_VALUE_RECORD_PLAY_OFFSET: {
            return (int) ((mRecorderRead - mPlayerRead - mAlignedRecordPlayOffset) * 1000L / mSampleRate);
        }
        case CFG_VALUE_RECORD_PRE_PROCESS: {
            return (int) (*((int*)&mConfigRecordLevelPreProcess));
        }
        case CFG_VALUE_SAMPLES_PER_FRAME: {
            return (int) (mSamplesPerFrame);
        }
        case CFG_VALUE_DEVICE_LATENCY: {
            return mDeviceLatency;
        }
        case CFG_VALUE_LIVE_LATENCY: {
            return ESTIMATED_DELAY + (mConfigRecPlayOffset * 1000 / mSampleRate);
        }
        //yangrui 0814
        case CFG_ACTION_HANDS_FREE:{
            return (int) mConfigHandsFree;
        }
        case CFG_ACTION_NOISE_DETECT: {
            NativeVoiceCodec* ptrCodec = mVoiceCodec;
            if (ptrCodec) {
               return ptrCodec->getCodecState(TYPE_NOISE_DETECT);
            }
            return 0;
        }
        case CFG_ACTION_DENOISE_MODE: {
            NativeVoiceCodec* ptrCodec = mVoiceCodec;
            if (ptrCodec) {
                return ptrCodec->getCodecState(TYPE_DENOISE_MODE);
            }
            return 0;
        }
        case CFG_VALUE_XD_COHERENCE:
        case CFG_VALUE_XE_COHERENCE:
        case CFG_VALUE_AEC_RATIO:{
         LOGI("configID : %d", configID);
        NativeVoiceCodec* ptrCodec = mVoiceCodec;
                if (ptrCodec) {
                    return  ptrCodec->getCodecState(configID);
                }
                else{
                    return 0;
                 }
        }
      }
    } else {
        NativeVoiceCodec* ptrCodec = mVoiceCodec;
        if (ptrCodec) {
            return ptrCodec->getCodecState(configID);
        }
    }

    return 0;
}

bool YeeCallRecorder::runEncoding(JNIEnv * env, jclass clzz, jobject jobj) {
    // setup jni env: find notify data available method in java class
    jobject javaObject = env->NewGlobalRef(jobj);
    if (!javaObject) {
        LOGE("Can't create global ref for recorder object");
    }

    // public boolean notifyEvent(int eventId, int counter)
    // descriptor: (II)Z
    jmethodID methodNotifyEvent = env->GetMethodID(clzz, "notifyEvent", "(II)Z");
    if (methodNotifyEvent == NULL) {
        LOGE("Can't find method notifyEvent. please add proguard rule to keep");
        return false;
    }

    // public int notifyPlayDataNeeded(byte[] data);
    // descriptor: ()Z
    jmethodID methodNotifyPlayDataNeeded = env->GetMethodID(clzz, "notifyPlayDataNeeded", "()Z");
    if (methodNotifyPlayDataNeeded == NULL) {
        LOGE("Can't find method notifyPlayDataNeeded. please add proguard rule to keep");
        return false;
    }

    // public void notifyRecordDataAvailable(int seq, byte[] data, int dataLen);
    // descriptor: (I[BI)V
    jmethodID methodNotifyRecordDataAvailable = env->GetMethodID(clzz, "notifyRecordDataAvailable", "(I[BI)V");
    if (methodNotifyRecordDataAvailable == NULL) {
        LOGE("Can't find method notifyRecordDataAvailable. please add proguard rule to keep");
        return false;
    }

    const jbyteArray incomingFrame = env->NewByteArray(4096);
    if (!incomingFrame) {
        LOGE("could not allocate incoming buffer. failed to run encoding");
        return false;
    }

    const jbyteArray outgoingFrame = env->NewByteArray(4096);
    if (!outgoingFrame) {
        LOGE("could not allocate outgoing buffer. failed to run encoding");
        return false;
    }

    LOGI("now entering recording loop %p, init=%d, prep=%d, record=%d", this, mInitialized, mPrepared, mRecording);

    if (!mInitialized || !mPrepared) {
        LOGI("recorder initialize failed, abort recording");
        env->DeleteLocalRef(incomingFrame);
        env->DeleteLocalRef(outgoingFrame);
        env->DeleteGlobalRef(javaObject);
        return false;
    }

    if (mInitialized && mPrepared && !mRecording) {
        // wait recording to start
        mSignal.wait();
    }

    SLAuint64 recorderLevel = 0;
    int recorderLevelMax = 0;
    jboolean isCopy = false;

    SLAint64 lastRequestPlayData = 0;
    SLAint64 lastRequestPlayEvent = 0;
    SLAint64 lastRequestPlayNoTraffic = 0;

    while (mRecording) {
        CHECK_BOUNDRY;

        if (false) {
            LOGI("dump queue - loop in encode - recorderRead: %lld, recorderWrite: %lld, playerRead: %lld, playerWrite: %lld, playerPending: %lld, offset: %d, jbRead: %lld, jbWrite: %lld",
                 mRecorderRead, mRecorderWrite, mPlayerRead, mPlayerWrite, mPlayerPending, mConfigRecPlayOffset, mPendingRead, mPendingWrite);
        }

        //------------------------------------------------------------------------------------------
        // step1: check & handle all pending events
        //
        // 1. mRequestPlaybackData: check if there is enough playing buffer pending. playing has higher priority
        // 2. mRequestPlaybackEvent: send playback continues event every 0.5 seconds to update ui
        // 3. mRequestPlaybackNoTraffic: send when lack of playback data every 6 seconds. request the peer with "Call:RESET" command
        // 4. mRequestPlaybackTrafficOK: traffic now ok
        //

        if (mRequestPlaybackData) {
            if (mPlayerRead - lastRequestPlayData > REQUEST_PLAYBACK_FRAME_COUNT * mSamplesPerFrame) {
                bool hasData = false;

                if (mPlayerPending - mPlayerWrite < mSamplesPerFrame) {
                    hasData = env->CallBooleanMethod(javaObject, methodNotifyPlayDataNeeded);
                    // LOGI("send request to java for more playing data: last=%lld, playerRead=%lld, hasData=%d", lastRequestPlayData, mPlayerRead, hasData);
                }

                mRequestPlaybackData = false;
                lastRequestPlayData = mPlayerRead;

                continue; // processed continue to process next async event
            }
        }

        if (mRequestPlaybackEvent) {
            mRequestPlaybackEvent = false;
            bool result = env->CallBooleanMethod(javaObject, methodNotifyEvent, SL_EVENT_PLAYING_CONTINUES, (int) mPlayerFrameCount);
            // LOGI("notify playing continues: %d", result);
            continue;
        }

        if (mRequestPlaybackNoTraffic) {
            mRequestPlaybackNoTraffic = false;

            SLAuint64 duration = mPlayerFrameCount - mNoTrafficSince;
            if (duration > SEND_PLAYBACK_NO_TRAFFIC_FRAME_COUNT) {
                int noTrafficCount = (int) (duration / SEND_PLAYBACK_NO_TRAFFIC_FRAME_COUNT);
                bool result = env->CallBooleanMethod(javaObject, methodNotifyEvent, SL_EVENT_NO_TRAFFIC, noTrafficCount);
                LOGI("notify playing no traffic: %d, count: %d", result, noTrafficCount);
                continue;
            }
        }

        if (mRequestPlaybackTrafficOK) {
            mRequestPlaybackTrafficOK = false;
            mNoTrafficSince = 0;
            mNoTrafficLastReport = 0;

            bool result = env->CallBooleanMethod(javaObject, methodNotifyEvent, SL_EVENT_TRAFFIC_OK, (int) mNoTrafficFrameCount);
            mNoTrafficFrameCount = 0;

            // LOGI("notify playing traffic resumed: %d, %d", result, mNoTrafficFrameCount);
            continue;
        }

        //------------------------------------------------------------------------------------------
        // step2: check if we can do encode with matched queue

        if (false) {
            LOGI("dump queue2 - loop in encode - recorderRead: %lld, recorderWrite: %lld, playerRead: %lld, playerWrite: %lld, playerPending: %lld, offset: %d, jbRead: %lld, jbWrite: %lld",
                 mRecorderRead, mRecorderWrite, mPlayerRead, mPlayerWrite, mPlayerPending, mConfigRecPlayOffset, mPendingRead, mPendingWrite);
        }

        if (mCurrentStallLevel > 0) {
            const int posPlay = mPlayerRead % mPlayerSize;
            const int posRecord = mRecorderRead % mRecorderSize;

            short* const farEndData = mPlayerBuffer + posPlay;
            short* const nearEndData = mRecorderBuffer + posRecord;

            mCurrentStallLevel--;

            // clear mic data, do not let peer hear the in stable echo voice
            memset(nearEndData, 0, mBytesPerFrame);

            // LOGI("recover from stall: %d remains", mCurrentStallLevel);

            if (mCurrentStallLevel == 0) {
                mAlignedRecordPlayOffset = mRecorderWrite - mPlayerWrite;
                LOGI("recover from record/play stall: record = %lld, play = %lld, aligned = %d", mRecorderWrite, mPlayerWrite, mAlignedRecordPlayOffset);
            }
        }

        // if recorded a frame
        if (mRecorderRead + mSamplesPerFrame <= mRecorderWrite) {
            const int frameSeqId = mRecorderRead / mSamplesPerFrame;

            if (false) {
                LOGI("encode: playerWrite=%lld, play queueLen=%lld, latency from aec = %d ms, rec/play offset = %d ms, device latency = %d ms, producerDiff=%lld, consumerDiff=%lld, diff=%lld"
                     , mPlayerWrite, (mPendingWrite - mPendingRead), ESTIMATED_DELAY, (mConfigRecPlayOffset * 1000 / mSampleRate), mDeviceLatency, (mRecorderWrite - mPlayerWrite), (mRecorderRead - mPlayerRead), (mRecorderWrite - mPlayerWrite) - (mRecorderRead - mPlayerRead));
            }

            // adjust mPlayerRead for reading reference voice data, keep in sync
            const SLint32 recPlayOffset = mConfigRecPlayOffset + (mRecorderWrite - mPlayerWrite);

            if (mRecorderRead - mPlayerRead != recPlayOffset) {
                SLAint64 old = mPlayerRead;
                mPlayerRead = mRecorderRead - recPlayOffset;

                if (false) {
                    LOGI("adjust played frame - recorderRead: %lld, recorderWrite: %lld, playerRead: %lld, playerWrite: %lld, playerPending: %lld, offset: %d, aligned: %d, total: %d, old: %lld",
                         mRecorderRead, mRecorderWrite, mPlayerRead, mPlayerWrite, mPlayerPending, mConfigRecPlayOffset, mAlignedRecordPlayOffset, recPlayOffset, old);
                }
            }

            const int posPlay = mPlayerRead % mPlayerSize;
            const int posRecord = mRecorderRead % mRecorderSize;

            // LOGI("posPlay=%d, posRecord=%d", posPlay, posRecord);
            // note: since mPlayerBuffer&mRecorderBuffer are cyclic buffers, so we need to assemble a full frame at last since rec/play are not aligned with frame border

            if (posPlay + mSamplesPerFrame > mPlayerSize) {
                // copy part of the the last frame since frame is not aligned to frame border, copy more data is ok
                memcpy(mPlayerBuffer + mPlayerSize, mPlayerBuffer, mBytesPerFrame);
            }

            if (posRecord + mSamplesPerFrame > mRecorderSize) {
                // copy part of the the last frame since frame is not aligned to frame border, copy more data is ok
                memcpy(mRecorderBuffer + mRecorderSize, mRecorderBuffer, mBytesPerFrame);
            }

            if (mDebugModeVerbose) {
                short* const farEndData = mPlayerBuffer + posPlay;
                short* const nearEndData = mRecorderBuffer + posRecord;

                int localRecLevel = 0;
                int localRecLevelMax = 0;

                // calculate raw recording level by 1/8 samples
                for (int i = 0; i < mSamplesPerFrame; i += 8) {
                    short sample = nearEndData[i];
                    if (sample != 0) {
                        int level = 0;
                        if (sample > 0) {
                            level += sample;
                        } else {
                            level -= sample;
                        }

                        if (localRecLevelMax < level) {
                            localRecLevelMax = level;
                        }
                        localRecLevel += level;
                    }
                }

                recorderLevel += localRecLevel;
                if (recorderLevelMax < localRecLevelMax) {
                    recorderLevelMax = localRecLevelMax;
                }

                LOGI("do encode - recorderRead: %lld, recorderWrite: %lld, recorderDiff: %lld, playerRead: %lld, playerWrite: %lld, playerPending: %lld, playerDiff: %lld, level=%0.4f%%, maxLevel=%d",
                     mRecorderRead, mRecorderWrite, (mRecorderWrite - mRecorderRead), mPlayerRead, mPlayerWrite, mPlayerPending, (mPlayerPending - mPlayerWrite), (localRecLevel * 8 * 100.0f / mSamplesPerFrame / 32767), localRecLevelMax);
            }

            latencyMeasurer* measurer = mLatencyMeasurer;
            if (measurer) {
                // send pure silence if we are running latency measuring
                //short* const farEndData = mPlayerBuffer + posPlay;
                short* const nearEndData = mRecorderBuffer + posRecord;

                //memset(farEndData, 0, mBytesPerFrame);
                memset(nearEndData, 0, mBytesPerFrame);
            }

            // audio pre-process: recording levels
            if (mConfigRecordLevelPreProcessEnabled) {
                short* const farEndData = mPlayerBuffer + posPlay;
                short* const nearEndData = mRecorderBuffer + posRecord;

                for (int i = 0; i < mSamplesPerFrame; i++) {
                    short val = nearEndData[i];
                    if (val) {
                        nearEndData[i] = val * mConfigRecordLevelPreProcess + 0.5f;
                    }
                }
            }

            // do encode
            NativeVoiceCodec* ptrCodec = mVoiceCodec;
            if (ptrCodec) {
                short* const farEndData = mPlayerBuffer + posPlay;
                short* const nearEndData = mRecorderBuffer + posRecord;

                jbyte* rawPtr = (jbyte*) env->GetByteArrayElements(outgoingFrame, &isCopy); // do not copy

                // int encode(jshort* farEndData, jshort* nearEndData, int msInSndCardBuf, jbyte* encodedData, int maxEncodeLen);
                int encodedLen = ptrCodec->encode(env, farEndData, nearEndData, 0, rawPtr, 4096);
                env->ReleaseByteArrayElements(outgoingFrame, rawPtr, 0);
                if (encodedLen > 0) {
                    // public void notifyRecordDataAvailable(int seq, byte[] data, int dataLen);
                    env->CallVoidMethod(javaObject, methodNotifyRecordDataAvailable, frameSeqId, outgoingFrame, encodedLen);
                } else {
                    LOGI("error: encoded data len < 0");
                }

            }

            if (mDebugLoopRecPlay) {
                // debug helper: loop to output

                if (mEchoCanceler) {
                    short* const farEndData = mPlayerBuffer + posPlay;
                    short* const nearEndData = mRecorderBuffer + posRecord;


                LOGI("%s_%d",__FILE__,__LINE__);
                    mEchoCanceler->aecmBufferFarEnd(farEndData, mSamplesPerFrame);
                    mEchoCanceler->aecmProcess(nearEndData, nearEndData, mSamplesPerFrame, 20);
                }

                const int posPending = mPlayerPending % mPlayerSize;
                mPlayerPending += mSamplesPerFrame;
                memcpy(mPlayerBuffer + posPending, mRecorderBuffer + posRecord, mBytesPerFrame);

                if (mDebugMode) {
                    LOGI("loop rec / play for debuging ...");
                }
            }

            // finally, we consumed the 2 buffer
            mPlayerRead += mSamplesPerFrame;
            mRecorderRead += mSamplesPerFrame;

            const int newPosPlay = mPlayerRead % mPlayerSize;
            const int newPosRecord = mRecorderRead % mRecorderSize;

            // LOGI("loop in encoding %d - play: %d --> %d, total: %d - record: %d --> %d, total: %d", frameSeqId, posPlay, newPosPlay, (newPosPlay - posPlay), posRecord, newPosRecord, (newPosRecord - posRecord));

#ifdef _DEBUG_MEMORY_BUFFER
            for (int i = posRecord; i < newPosRecord; i++) {
                mRecorderBuffer[i] = 0xfeed; // clear the original buffer for debug
            }
#endif
            continue;
        }

        // loop to here: wait for recorded data, until signaled (new data recorded), set timeout in RECORDER_DURATION_MS to check again
        mSignal.wait(RECORDER_DURATION_MS);

        if (false) {
            bool hasData = (mRecorderRead + mSamplesPerFrame <= mRecorderWrite)
                           && (mPlayerRead + mSamplesPerFrame <= mPlayerWrite)
                           && (mRecorderRead - mPlayerRead == mConfigRecPlayOffset);

            if (!hasData && !mRequestPlaybackData && !mRequestPlaybackEvent && !mRequestPlaybackNoTraffic && !mRequestPlaybackTrafficOK) {
                LOGI("dump queue - wait exit with no data - recorderRead: %lld, recorderWrite: %lld, playerRead: %lld, playerWrite: %lld, playerPending: %lld",
                     mRecorderRead, mRecorderWrite, mPlayerRead, mPlayerWrite, mPlayerPending);
            }
        }
    }

    env->DeleteLocalRef(incomingFrame);
    env->DeleteLocalRef(outgoingFrame);
    env->DeleteGlobalRef(javaObject);

    LOGI("now exit recording loop %p, init=%d, prep=%d, record=%d, recordLevel=%0.4f%%, recordMax=%d",
         this, mInitialized, mPrepared, mRecording, (recorderLevel * 8 * 100.0f / mRecorderRead / 32767), recorderLevelMax);

    return true;
}

bool YeeCallRecorder::playDataAvailable(char* buffer, int len) {
    if (mDebugModeVerbose) {
        LOGI("play data: id=%lld, queueLen=%lld, %p, %d - %s", mPendingWrite, (mPendingWrite - mPendingRead), buffer, len, dumpMemory(buffer, len));
    }

    const int posWrite = mPendingWrite % PENDING_FRAME_COUNT;

    PlayFrame* frame = mPendingBuffer + posWrite;
    frame->frameId = mPendingWrite;
    frame->length = len;
    memcpy(frame->data, buffer, len);

    // write the frame & move to next position
    mPendingWrite++;

    if (mPendingWrite - mPendingRead > 100) {
        LOGI("no enough buffer for playing. skip now: pendingWrite=%lld, pendingRead=%lld, pending=%lld", mPendingWrite, mPendingRead, (mPendingWrite - mPendingRead));
        mPendingRead = mPendingWrite - (PENDING_FRAME_COUNT / 2);
        mFadeInPlay = 20;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool YeeCallRecorder::createOpenSLEngine() {
    // create engine
    RETURNIF_ERROR_BOOL(slCreateEngine(&mEngineObject, 0, NULL, 0, NULL, NULL));

    // realize the engine
    RETURNIF_ERROR_BOOL((*mEngineObject)->Realize(mEngineObject, SL_BOOLEAN_FALSE));

    // get the engine interface, which is needed in order to create other objects
    RETURNIF_ERROR_BOOL((*mEngineObject)->GetInterface(mEngineObject, SL_IID_ENGINE, &mEngineEngine));

    // setup initial values for our recording
    mInitialized = true;
    mPrepared = false;
    mRecording = false;

    // create codec: default to opus now
    mVoiceCodec = new NativeVoiceCodec();
    if (mVoiceCodec->open(CODEC_OPUS, mSampleRate, 0) != 0) {
        LOGI("failed to open audio codec.");
        return false;
    }

    // create latency measurer
    mLatencyMeasurer = NULL;

    // AEC for rec/play loop
    if (mDebugLoopRecPlay) {
        mEchoCanceler = new NativeWebRtcAEC();
        if (mEchoCanceler->aecmInit(mSampleRate) != 0) {
            LOGI("failed to init echo canceler.");
            return false;
        }
    }

    return true;
}

bool YeeCallRecorder::createAudioPlayer() {
    SLresult result;

    // malloc playing buffers
    mPlayerSize = BUFFER_COUNT_MAX * mSamplesPerFrame;
    mPlayerRead = 0;
    mPlayerWrite = 0;

    mPendingSize = PENDING_FRAME_COUNT;
    mPendingRead = 0;
    mPendingWrite = 0;
    mInJitterRecovery = -1;

    // clear the buffer
    memset(mPlayerBuffer, 0, mPlayerSize * sizeof(short));
    memset(mPendingBuffer, 0, sizeof(mPendingBuffer));

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq;
    loc_bufq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    loc_bufq.numBuffers = 2; // mConfigRecordBufferSize > 0 ? 2 : 1; // single buffer is ok for OS >= 4.4, to get optimal latency

    // 8KHz / 16KHz mono PCM 16bit
    SLDataFormat_PCM format_pcm;
    format_pcm.formatType = SL_DATAFORMAT_PCM;
    format_pcm.numChannels = 1;
    format_pcm.samplesPerSec = (mSampleRate == 16000) ? SL_SAMPLINGRATE_16 : SL_SAMPLINGRATE_8;
    format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    if (mOutputMixObject != NULL) {
        (*mOutputMixObject)->Destroy(mOutputMixObject);
        mOutputMixObject = NULL;
    }

    // create output mix for audio sink
    RETURNIF_ERROR_BOOL((*mEngineEngine)->CreateOutputMix(mEngineEngine, &mOutputMixObject, 0, NULL, NULL));

    // realize the output mix
    RETURNIF_ERROR_BOOL((*mOutputMixObject)->Realize(mOutputMixObject, SL_BOOLEAN_FALSE));

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix;
    loc_outmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    loc_outmix.outputMix = mOutputMixObject;

    SLDataSink audioSnk;
    audioSnk.pLocator = &loc_outmix;
    audioSnk.pFormat = NULL;

    // create audio player
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE};

    if (mPlayerObject != NULL) {
        (*mPlayerObject)->Destroy(mPlayerObject);
        mPlayerObject = NULL;
        mPlayerPlay = NULL;
        mPlayerBufferQueue = NULL;
        mPlayerVolume = NULL;
    }

    RETURNIF_ERROR_BOOL((*mEngineEngine)->CreateAudioPlayer(mEngineEngine,
                        &mPlayerObject,
                        &audioSrc,
                        &audioSnk,
                        3,
                        ids,
                        req));

    // configure the player
    SLAndroidConfigurationItf playerConfig;
    RETURNIF_ERROR_BOOL((*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_ANDROIDCONFIGURATION, &playerConfig));

    // a hint value from configuration file, must be set before realize the object
    SLuint32 presetValue = mConfigPlayStream;
    RETURNIF_ERROR_BOOL((*playerConfig)->SetConfiguration(playerConfig, SL_ANDROID_KEY_STREAM_TYPE, &presetValue, sizeof(SLuint32)));

    // realize the player
    RETURNIF_ERROR_BOOL((*mPlayerObject)->Realize(mPlayerObject, SL_BOOLEAN_FALSE));

    // get the play interface
    RETURNIF_ERROR_BOOL((*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_PLAY, &mPlayerPlay));

    // get the buffer queue interface
    RETURNIF_ERROR_BOOL((*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_BUFFERQUEUE, &mPlayerBufferQueue));

    // register callback on the buffer queue
    RETURNIF_ERROR_BOOL((*mPlayerBufferQueue)->RegisterCallback(mPlayerBufferQueue, openslPlayerCallback, this));

    // get the optional volume interface
    WARNIF_ERROR((*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_VOLUME, &mPlayerVolume));

    LOGI("player is now prepared with preset: %d", presetValue);
    return true;
}

// create audio recorder
bool YeeCallRecorder::createAudioRecorder() {
    // checkout optimal recording buffer size for this device

    // malloc recording buffers
    mRecorderSize = BUFFER_COUNT_MAX * mSamplesPerFrame;
    mRecorderRead = 0;
    mRecorderWrite = 0;

#ifdef _DEBUG_MEMORY_BUFFER
    // init the buffer for debug
    for (int i = 0; i < mRecorderSize; i++) {
        mRecorderBuffer[i] = 0xfeed;
    }
#else
    memset(mRecorderBuffer, 0, mRecorderSize * sizeof(short));
#endif

    // configure audio source
    SLDataLocator_IODevice loc_dev;
    loc_dev.locatorType = SL_DATALOCATOR_IODEVICE;
    loc_dev.deviceType = SL_IODEVICE_AUDIOINPUT;
    loc_dev.deviceID = SL_DEFAULTDEVICEID_AUDIOINPUT;
    loc_dev.device = NULL;

    SLDataSource audioSrc = {&loc_dev, NULL};

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq;
    loc_bq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    loc_bq.numBuffers = 2;

    // 8KHz / 16KHz mono PCM 16bit
    SLDataFormat_PCM format_pcm;
    format_pcm.formatType = SL_DATAFORMAT_PCM;
    format_pcm.numChannels = 1;
    format_pcm.samplesPerSec = (mSampleRate == 16000) ? SL_SAMPLINGRATE_16 : SL_SAMPLINGRATE_8;
    format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    if (mRecorderObject != NULL) {
        (*mRecorderObject)->Destroy(mRecorderObject);
        mRecorderObject = NULL;
        mRecorderRecord = NULL;
        mRecorderBufferQueue = NULL;
    }

    // create audio recorder (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[2] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION};
    const SLboolean req[2] = {SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE};
    RETURNIF_ERROR_BOOL((*mEngineEngine)->CreateAudioRecorder(mEngineEngine,
                        &mRecorderObject,
                        &audioSrc,
                        &audioSnk,
                        2,
                        id,
                        req));

    SLAndroidConfigurationItf recorderConfig;
    RETURNIF_ERROR_BOOL((*mRecorderObject)->GetInterface(mRecorderObject, SL_IID_ANDROIDCONFIGURATION, &recorderConfig));

    // a hint value from configuration file, must be set before realize the object
    SLuint32 presetValue = mConfigRecordMode;
    RETURNIF_ERROR_BOOL((*recorderConfig)->SetConfiguration(recorderConfig, SL_ANDROID_KEY_RECORDING_PRESET, &presetValue, sizeof(SLuint32)));

    // realize the audio recorder
    RETURNIF_ERROR_BOOL((*mRecorderObject)->Realize(mRecorderObject, SL_BOOLEAN_FALSE));

    // get the record interface
    RETURNIF_ERROR_BOOL((*mRecorderObject)->GetInterface(mRecorderObject, SL_IID_RECORD, &mRecorderRecord));

    // get the buffer queue interface
    RETURNIF_ERROR_BOOL((*mRecorderObject)->GetInterface(mRecorderObject,
                        SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                        &mRecorderBufferQueue));

    // register callback on the buffer queue
    RETURNIF_ERROR_BOOL((*mRecorderBufferQueue)->RegisterCallback(mRecorderBufferQueue, openslRecorderCallback, this));

    LOGI("recorder is now prepared with preset: %d", presetValue);
    return true;
}

// shut down the native audio system
bool YeeCallRecorder::destroyOpenSLEngine() {
    if (!mInitialized) {
        LOGE("error: the recorder object already destroied: %p", this);
        return false;
    }

    if (mRecording) {
        LOGE("error: record in progress. should stop befoire destory");
        stopRecording();
    }

    LOGI("now destroy opensl engine");

    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (mPlayerObject != NULL) {
        (*mPlayerObject)->Destroy(mPlayerObject);
        mPlayerObject = NULL;
        mPlayerPlay = NULL;
        mPlayerBufferQueue = NULL;
        mPlayerVolume = NULL;
    }

    // destroy audio recorder object, and invalidate all associated interfaces
    if (mRecorderObject != NULL) {
        (*mRecorderObject)->Destroy(mRecorderObject);
        mRecorderObject = NULL;
        mRecorderRecord = NULL;
        mRecorderBufferQueue = NULL;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (mOutputMixObject != NULL) {
        (*mOutputMixObject)->Destroy(mOutputMixObject);
        mOutputMixObject = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (mEngineObject != NULL) {
        (*mEngineObject)->Destroy(mEngineObject);
        mEngineObject = NULL;
        mEngineEngine = NULL;
    }

    // recorded audio at 8/16 kHz mono, 16-bit signed little endian
    mRecorderFrameCount = 0;
    mRecorderSize = 0;
    mRecorderRead = 0;
    mRecorderWrite = 0;

    // continuous buffer of playing audio at 8/16 kHz mono, 16-bit signed little endian
    mPlayerFrameCount = 0;
    mPlayerFakeFrameCount = 0;
    mSkippedFrameCount = 0;
    mPlayerSize = 0;     // samples count for buffer
    mPlayerRead = 0;     // total samples read
    mPlayerWrite = 0;    // total samples write
    mPlayerPending = 0;

    NativeVoiceCodec* ptrCodec = mVoiceCodec;
    mVoiceCodec = NULL;
    if (ptrCodec) {
        ptrCodec->close();
        delete ptrCodec;
    }

    IAcousticEchoCanceler* echoCanceler = mEchoCanceler;
    if(mEchoCanceler) {
        mEchoCanceler->aecmClose();
    }
    mEchoCanceler = NULL;
    if (echoCanceler) {
        delete echoCanceler;
    }

    latencyMeasurer* measurer = mLatencyMeasurer;
    mLatencyMeasurer = NULL;
    if (measurer) {
        delete measurer;
    }

    mRequestPlaybackData = false;
    mRequestPlaybackEvent = false;
    mRequestPlaybackNoTraffic = false;
    mRequestPlaybackTrafficOK = false;
    mNoTrafficSince = 0;
    mNoTrafficLastReport = 0;
    mNoTrafficFrameCount = 0;

    mLastPlayPosForRecord = 0;
    mLastRecordPosForPlay = 0;

    mLastTimeForRecord = 0;
    mLastTimeForPlay = 0;

    mStallLevelRecord = 0;
    mStallLevelPlay = 0;

    memset(mFrameDurationPlay, 0, sizeof(mFrameDurationPlay));
    memset(mFrameDurationRecord, 0, sizeof(mFrameDurationRecord));

    mFrameDurationPlayPos = 0;
    mFrameDurationRecordPos = 0;

    mCurrentStallLevel = 0;
    mAlignedRecordPlayOffset = 0;

    mPrevLatencyFromAEC = 0;
    mSameLatencyCount = 0;

    memset(mEstimatedLatency, 0, sizeof(mEstimatedLatency));
    mDeviceLatency = 0;
    mLatencyCount = 0;

    mProducerConsumerAlignment = -1;

    mInitialized = false;
    mPrepared = false;
    mRecording = false;

    return true;
}

void YeeCallRecorder::applyAudioLatency(float latency) {
    if (latency < 20) {
        return;
    }

    mKnowLatency = latency;
}

SLAint64 YeeCallRecorder::currentTime() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

int YeeCallRecorder::calculateAudioLatency() {
    // SLuint32 mEstimatedLatency[LATENCY_SLOT]; // estimated device latency from aec module

    int nonZeroCount = 0;
    int maxCount = 0;
    int maxPos = -1;
    int totalCount = 0;

    for (int i = 0; i < LATENCY_SLOT; i++) {
        SLuint32 value = mEstimatedLatency[i];
        if (value) {
            nonZeroCount++;
        }

        if (maxCount < value) {
            maxCount = value;
            maxPos = i;
        }

        totalCount += value;
    }

    int estimatedLatency = maxPos * 4 + mConfigRecPlayOffset * 1000 / mSampleRate;
    LOGI("resolve latency: nonZero=%d, maxCount=%d, maxPos=%d, totalCount=%d, latency=%d ms, recent latency = %d, recent count = %d",
         nonZeroCount, maxCount, maxPos, totalCount, estimatedLatency, mPrevLatencyFromAEC, mSameLatencyCount);

    if (estimatedLatency > 500) {
        LOGI("estimated latency out of range. reset & ignore");
        mDeviceLatency = 0;
        mLatencyCount = 0;
        mConfigRecPlayOffset = 0;
        return 0;
    }

    // resolve latency: nonZero=6, maxCount=4, maxPos=0, totalCount=12, latency=0 ms
    if (nonZeroCount > 15 || maxCount < 10 || totalCount < 10 || (maxCount * 1.0f / totalCount < 0.5f) || mSameLatencyCount < 3) {
        LOGI("data dropped due to additional stability rule check mismatch. dominate factor: %f", (maxCount * 1.0f / totalCount));
        return -1;
    }

    if (mSameLatencyCount >= 30) {
        int latency = mPrevLatencyFromAEC + mConfigRecPlayOffset * 1000 / mSampleRate;
        if (abs(estimatedLatency - latency) < 20) {
            LOGI("aec enters stable state with 60 seconds: %d", latency);
            return latency;
        }
    }

    return estimatedLatency;
}

void YeeCallRecorder::initBufferBoundry() {
    mRecorderBufferBegin = &mRecorderBufferBegin;
    mRecorderBufferEnd = &mRecorderBufferEnd;
    mPlayerBufferBegin = &mPlayerBufferBegin;
    mPlayerBufferEnd = &mPlayerBufferEnd;
}

void YeeCallRecorder::checkBufferBoundry(const char * const funcName, int line) {
    // additional boundry checked since these buffer used in system api memory space

    if (mRecorderBufferBegin != &mRecorderBufferBegin) {
        LOGE("recorder begin check failure: %s:%d - %s", funcName, line, dumpMemory((char*)&mRecorderBufferBegin, 2048));
    }
    if (mRecorderBufferEnd != &mRecorderBufferEnd) {
        LOGE("recorder end check failure: %s:%d - %s", funcName, line, dumpMemory((char*)&mRecorderBufferEnd, 128));
    }
    if (mPlayerBufferBegin != &mPlayerBufferBegin) {
        LOGE("player begin check failure: %s:%d - %s",funcName, line, dumpMemory((char*)&mPlayerBufferBegin, 2048));
    }
    if (mPlayerBufferEnd != &mPlayerBufferEnd) {
        LOGE("player end check failure: %s:%d - %s", funcName, line, dumpMemory((char*)&mPlayerBufferEnd, 128));
    }

    // LOGI("no error found");
}

//---------------------------------------------------------------------------------------------------------------------
