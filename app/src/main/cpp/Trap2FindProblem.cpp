//
// Created by yeeapp on 2019-07-09.
//

#include "Trap2FindProblem.h"

#include <jni.h>
#include <string>
#include <assert.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>

#define AUDIO_SRC_PATH "/sdcard/audio.pcm"
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"haohao",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"haohao",FORMAT,##__VA_ARGS__);
#define NUM_RECORDER_EXPLICIT_INTERFACES 2
#define NUM_BUFFER_QUEUE 1
#define SAMPLE_RATE 44100
#define PERIOD_TIME 20  // 20ms
#define FRAME_SIZE SAMPLE_RATE * PERIOD_TIME / 1000
#define CHANNELS 2
#define BUFFER_SIZE   (FRAME_SIZE * CHANNELS)
// engine interfaces
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine = NULL;
// audio recorder interfaces
static SLObjectItf recorderObject = NULL;
static SLRecordItf recorderRecord = NULL;
static SLAndroidSimpleBufferQueueItf recorderBuffQueueItf = NULL;
static SLAndroidConfigurationItf configItf = NULL;
// pcm audio player interfaces
static SLObjectItf playerObject = NULL;
static SLPlayItf playerPlay = NULL;
static SLObjectItf outputMixObjext = NULL; // 混音器
static SLAndroidSimpleBufferQueueItf playerBufferQueueItf = NULL;

void createEngine() {
    SLEngineOption EngineOption[] = {
            {(SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE}
    };
    SLresult result;
    result = slCreateEngine(&engineObject, 1, EngineOption, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    /* Realizing the SL Engine in synchronous mode. */
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);
}

class AudioContext {
public:
    FILE *pfile;
    uint8_t *buffer;
    size_t bufferSize;

    AudioContext(FILE *pfile, uint8_t *buffer, size_t bufferSize) {
        this->pfile = pfile;
        this->buffer = buffer;
        this->bufferSize = bufferSize;
    }
};

static AudioContext *recorderContext = NULL;

// 录制音频时的回调
void AudioRecorderCallback(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *context) {
    AudioContext *recorderContext = (AudioContext *) context;
    assert(recorderContext != NULL);
    if (recorderContext->buffer != NULL) {
        fwrite(recorderContext->buffer, recorderContext->bufferSize, 1, recorderContext->pfile);
        LOGI("save a frame audio data.");
        SLresult result;
        SLuint32 state;
        result = (*recorderRecord)->GetRecordState(recorderRecord, &state);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;
        if (state == SL_RECORDSTATE_RECORDING) {
            result = (*bufferQueueItf)->Enqueue(bufferQueueItf, recorderContext->buffer, recorderContext->bufferSize);
            assert(SL_RESULT_SUCCESS == result);
            (void) result;
        }
    }
}

// 播放音频时的回调
void AudioPlayerCallback(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *context) {
    AudioContext *playerContext = (AudioContext *) context;
    if (!feof(playerContext->pfile)) {
        fread(playerContext->buffer, playerContext->bufferSize, 1, playerContext->pfile);
        LOGI("read a frame audio data.");
        (*bufferQueueItf)->Enqueue(bufferQueueItf, playerContext->buffer, playerContext->bufferSize);
    } else {
        fclose(playerContext->pfile);
        delete playerContext->buffer;
    }
}

// 创建音频播放器
void createAudioPlayer(SLEngineItf engineEngine, SLObjectItf outputMixObject, SLObjectItf &audioPlayerObject) {
    SLDataLocator_AndroidSimpleBufferQueue dataSourceLocator = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
            1
    };
    // PCM 数据源格式
    SLDataFormat_PCM dataSourceFormat = {
            SL_DATAFORMAT_PCM,
            2,
            SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSource dataSource = {
            &dataSourceLocator,
            &dataSourceFormat
    };
    SLDataLocator_OutputMix dataSinkLocator = {
            SL_DATALOCATOR_OUTPUTMIX, // 定位器类型
            outputMixObject // 输出混合
    };
    SLDataSink dataSink = {
            &dataSinkLocator, // 定位器
            0,
    };
    // 需要的接口
    SLInterfaceID interfaceIDs[] = {
            SL_IID_BUFFERQUEUE
    };
    SLboolean requiredInterfaces[] = {
            SL_BOOLEAN_TRUE
    };
    // 创建音频播放对象
    SLresult result = (*engineEngine)->CreateAudioPlayer(
            engineEngine,
            &audioPlayerObject,
            &dataSource,
            &dataSink,
            1,
            interfaceIDs,
            requiredInterfaces
    );
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
}

void startRecord(){
    if ( engineEngine == NULL ) {
        createEngine();
    }
    // 创建混音器
    SLresult result;
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObjext, 0, 0, 0);
    assert(SL_RESULT_SUCCESS= result);
    (void)result;
    result = (*outputMixObjext)->Realize(outputMixObjext, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS== result);
    (void)result;
    FILE *p_file = fopen(AUDIO_SRC_PATH, "r");
    // 创建播放器
    createAudioPlayer(engineEngine, outputMixObjext, playerObject
    );
    result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS== result);
    (void)result;
    result = (*playerObject)->GetInterface(playerObject, SL_IID_BUFFERQUEUE,
                                       &playerBufferQueueItf);
    assert(SL_RESULT_SUCCESS== result);
    (void)result;
    uint8_t *buffer = new uint8_t[BUFFER_SIZE];
    AudioContext *playerContext = new AudioContext(p_file, buffer, BUFFER_SIZE);
    result = (*playerBufferQueueItf)->RegisterCallback(playerBufferQueueItf, AudioPlayerCallback,
                                                       playerContext);
    assert(SL_RESULT_SUCCESS== result);
    (void)result;
    result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay);
    assert(SL_RESULT_SUCCESS== result);
    (void)result;
    result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS== result);
    AudioPlayerCallback(playerBufferQueueItf, playerContext;
    }

// 停止播放音频
void AudioRecorder_stopPlay() {
    if (playerPlay != NULL) {
        SLresult result;
        result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_STOPPED);
        assert(SL_RESULT_SUCCESS == result);
    }

// 开始采集音频数据，并保存到本地
void startRecord() {
if (engineEngine == NULL) {
createEngine();

}
if (recorderObject != NULL) {
LOGI("Audio recorder already has been created.");
return;
}
FILE *p_file = fopen(AUDIO_SRC_PATH, "w");
if (p_file == NULL) {
LOGI("Fail to open file.");
return;
}
SLresult result;
/* setup the data source*/
SLDataLocator_IODevice ioDevice = {
        SL_DATALOCATOR_IODEVICE,
        SL_IODEVICE_AUDIOINPUT,
        SL_DEFAULTDEVICEID_AUDIOINPUT,
        NULL
};
SLDataSource recSource = {&ioDevice, NULL};
SLDataLocator_AndroidSimpleBufferQueue recBufferQueue = {
        SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
        NUM_BUFFER_QUEUE
};
SLDataFormat_PCM pcm = {
        SL_DATAFORMAT_PCM, // pcm 格式的数据
        2,  // 2 个声道（立体声）
        SL_SAMPLINGRATE_44_1, // 44100hz 的采样频率
        SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
        SL_BYTEORDER_LITTLEENDIAN
};
SLDataSink dataSink = {&recBufferQueue, &pcm};
SLInterfaceID iids[NUM_RECORDER_EXPLICIT_INTERFACES] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION};
SLboolean required[NUM_RECORDER_EXPLICIT_INTERFACES] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
/* Create the audio recorder */
result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &recSource, &dataSink,
                                              NUM_RECORDER_EXPLICIT_INTERFACES, iids, required);
assert(SL_RESULT_SUCCESS
== result);
/* get the android configuration interface*/
result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDCONFIGURATION, &configItf);
assert(SL_RESULT_SUCCESS
== result);
/* Realize the recorder in synchronous mode. */
result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
assert(SL_RESULT_SUCCESS
== result);
/* Get the buffer queue interface which was explicitly requested */
result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, (void *) &recorderBuffQueueItf);
assert(SL_RESULT_SUCCESS
== result);
/* get the record interface */
result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
assert(SL_RESULT_SUCCESS
== result);
uint8_t *buffer = new uint8_t[BUFFER_SIZE];
recorderContext = new AudioContext(p_file, buffer, BUFFER_SIZE);
result = (*recorderBuffQueueItf)->RegisterCallback(recorderBuffQueueItf, AudioRecorderCallback, recorderContext);
assert(SL_RESULT_SUCCESS
== result);
/* Enqueue buffers to map the region of memory allocated to store the recorded data */
result = (*recorderBuffQueueItf)->Enqueue(recorderBuffQueueItf, recorderContext->buffer, BUFFER_SIZE);
assert(SL_RESULT_SUCCESS
== result);
/* Start recording */
// 开始录制音频
result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
assert(SL_RESULT_SUCCESS
== result);
LOGI("Starting recording");
}



int Trap2FindProblem::messageno()
{
    return 200;
}