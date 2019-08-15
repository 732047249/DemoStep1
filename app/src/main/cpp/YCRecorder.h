//
// Created by yeeapp on 2019-07-11.
//

#ifndef DEMOSTEP1_YCRECORDER_H
#define DEMOSTEP1_YCRECORDER_H

#include <string>
#include <array>
#include <vector>
#include <cstdint>
#include <android/log.h>
// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>

#define AUDIO_SRC_PATH "/sdcard/audio111.pcm"
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"haohao",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"haohao",FORMAT,##__VA_ARGS__);
#define NUM_RECORDER_EXPLICIT_INTERFACES 2
#define NUM_BUFFER_QUEUE 1
#define SAMPLE_RATE 44100
#define PERIOD_TIME 20  // 20ms
#define FRAME_SIZE SAMPLE_RATE * PERIOD_TIME / 1000
#define CHANNELS 2
#define BUFFER_SIZE   (FRAME_SIZE * CHANNELS)

class YCRecorder {
public:
    YCRecorder();
    ~YCRecorder();
    bool init();
    void startRecord();


    void createAudioPlayer(SLEngineItf engineEngine, SLObjectItf outputMixObject, SLObjectItf &audioPlayerObject);
};


#endif //DEMOSTEP1_YCRECORDER_H
