/*
 *
 */



#include <android/log.h>
#include <assert.h>
#include <memory>
#include "AListener.h"

void AListener::main() {
    //log("MAIN");
    auto status = init();
    //log(std::string{"init status: "} + (status ? "ok" : "fail"));
    //log("RECORDING…");
    start_record();
    //log("FINISHED");
}

void AListener::log(std::string str) {
    __android_log_print(ANDROID_LOG_INFO, "AListener", "%s", str.c_str());
}

void AListener::write(AListener::record_buf_t *data) {
    static int count = 0;
    auto f = fopen(FNAME, "a");
    //auto f = fopen("/mnt/sdcard/android1.pcm", "a");
    //FILE* f = fopen(FNAME, "a");
    if (f) {
        fwrite(data->data.data(), 1, data->filled, f);
        __android_log_print(ANDROID_LOG_INFO, "AListener", "file written ok");

        fclose(f);
        __android_log_print(ANDROID_LOG_INFO, "AListener", "file close ok");
    }
    else {
        __android_log_print(ANDROID_LOG_INFO, "AListener", "failed to open file");
    }
}
// this callback handler is called every time a buffer finishes recording
void recorder_callback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    //AListener::log("recorder callback");
    auto buf = reinterpret_cast<AListener::record_buf_t*>(context);

    // for streaming recording, here we would call Enqueue to give recorder the next buffer to fill
    // but instead, this is a one-time buffer so we stop recording
    SLresult result;
    result = (*buf->rec)->SetRecordState(buf->rec, SL_RECORDSTATE_STOPPED);
    if (SL_RESULT_SUCCESS == result) {
        AListener::log("record success");
        buf->filled = AListener::RECORDER_FRAMES * sizeof(short);
    } else {
        //AListener::log("record fail");
    }

    AListener::write(buf);

    buf->listener->record_loop();
}

bool AListener::init( ){
    create_eng();
    rbuf.listener = this;

    SLresult result;

    // configure audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
                                      SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_16,
                                   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*engineEng)->CreateAudioRecorder(engineEng, &recorderObj, &audioSrc,
                                               &audioSnk, 1, id, req);
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    // realize the audio recorder
    result = (*recorderObj)->Realize(recorderObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    // get the start_record interface
    result = (*recorderObj)->GetInterface(recorderObj, SL_IID_RECORD, &rbuf.rec);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the buffer queue interface
    result = (*recorderObj)->GetInterface(recorderObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                          &recorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // register callback on the buffer queue
    result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, recorder_callback,
                                                      &rbuf);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    return true;
}

void AListener::create_eng() {
    SLresult result;

    // create engine
    result = slCreateEngine(&engineObj, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the engine
    result = (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engineEng);
    assert(SL_RESULT_SUCCESS == result);
}

void AListener::start_record() {
    auto result = (*rbuf.rec)->SetRecordState(rbuf.rec, SL_RECORDSTATE_STOPPED);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
    result = (*recorderBufferQueue)->Clear(recorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    record_loop();
}


void AListener::record_loop() {
    SLresult result;

    {
        static uint sec{0};
        if (++sec == 30) {
            //log("record loop finished");
            return;
        }
        //og("record loop in process");
    }

    // the buffer is not valid for playback yet
    rbuf.filled = 0;

    result = (*recorderBufferQueue)->Clear(recorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);

    // enqueue an empty buffer to be filled by the recorder
    // (for streaming recording, we would enqueue at least 2 empty buffers to start things off)
    result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, rbuf.data.data(),
                                             rbuf.data.size() * sizeof(short));
    // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
    // which for this code example would indicate a programming error
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // start recording
    result = (*rbuf.rec)->SetRecordState(rbuf.rec, SL_RECORDSTATE_RECORDING);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
}

