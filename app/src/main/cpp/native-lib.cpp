#include <jni.h>
#include <string>


#include <string>
#include <array>
#include <vector>
#include <cstdint>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>

#include <android/log.h>


#include "YCRecorder.h"

#include "AListener.h"

#include "./jni/NativeOpusCodec.h"

extern "C" JNIEXPORT jstring JNICALL
Java_sdkapp_video_lightsky_com_demostep1_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {



    std::string hello = "Hello from C++";

//    YCRecorder r;
//    r.startRecord();
//    NativeOpusCodec * pn = new NativeOpusCodec();
//
    l= new AListener();
    l->main();

    return env->NewStringUTF(hello.c_str());
}
