#include "com_zayhu_library_jni_OpenSLRecorder.h"
#include "YeeCallRecorder.h"
#include "logcat.h"

/*
 * Class:     com_zayhu_library_jni_OpenSLRecorder
 * Method:    nativeCreate
 * Signature: (IIIIII)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_OpenSLRecorder_nativeCreate
(JNIEnv * env, jclass clzz, jint sampleRate, jint recordBufferSize, jint flags, jint recordFlags, jint playFlags, jint recPlayOffset) {
    YeeCallRecorder* recorder = new YeeCallRecorder();

    if (recorder) {
        if (recorder->create(sampleRate, recordBufferSize, flags, recordFlags, playFlags, recPlayOffset)) {
            return (int) recorder;
        } else {
            delete recorder; // init failed.
        }
    }

    return 0;
}

/*
 * Class:     com_zayhu_library_jni_OpenSLRecorder
 * Method:    nativeDestroy
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_zayhu_library_jni_OpenSLRecorder_nativeDestroy
(JNIEnv * env, jclass clzz, jint nativeObject) {
    YeeCallRecorder* recorder = (YeeCallRecorder*) nativeObject;

    if (recorder) {
        recorder->destroy();
    }
}

/*
 * Class:     com_zayhu_library_jni_OpenSLRecorder
 * Method:    nativeLastError
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_OpenSLRecorder_nativeLastError
(JNIEnv * env, jclass clzz, jint nativeObject) {
    YeeCallRecorder* recorder = (YeeCallRecorder*) nativeObject;

    if (recorder) {
        return recorder->lastError();
    }

    return 0;
}

/*
 * Class:     com_zayhu_library_jni_OpenSLRecorder
 * Method:    nativePrepareRecording
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_OpenSLRecorder_nativePrepareRecording
(JNIEnv * env, jclass clzz, jint nativeObject) {
    YeeCallRecorder* recorder = (YeeCallRecorder*) nativeObject;

    if (recorder) {
        return recorder->prepareRecording();
    }

    return 0;
}


/*
 * Class:     com_zayhu_library_jni_OpenSLRecorder
 * Method:    nativeStartRecording
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_OpenSLRecorder_nativeStartRecording
(JNIEnv * env, jclass clzz, jint nativeObject) {
    YeeCallRecorder* recorder = (YeeCallRecorder*) nativeObject;

    if (recorder) {
        return recorder->startRecording();
    }

    return 0;
}


/*
 * Class:     com_zayhu_library_jni_OpenSLRecorder
 * Method:    nativeStopRecording
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_OpenSLRecorder_nativeStopRecording
(JNIEnv * env, jclass clzz, jint nativeObject) {
    YeeCallRecorder* recorder = (YeeCallRecorder*) nativeObject;

    if (recorder) {
        return recorder->stopRecording();
    }

    return 0;
}

/*
 * Class:     com_zayhu_library_jni_OpenSLRecorder
 * Method:    nativeSetConfig
 * Signature: (III)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_OpenSLRecorder_nativeSetConfig
(JNIEnv * env, jclass clzz, jint nativeObject, jint configID, jint value) {
    YeeCallRecorder* recorder = (YeeCallRecorder*) nativeObject;

    if (recorder) {
        return recorder->setConfig(configID, value);
    }

    return 0;
}

/*
 * Class:     com_zayhu_library_jni_OpenSLRecorder
 * Method:    nativeGetConfig
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_OpenSLRecorder_nativeGetConfig
(JNIEnv * env, jclass clzz, jint nativeObject, jint configID)
{
    YeeCallRecorder* recorder = (YeeCallRecorder*) nativeObject;

    if (recorder) {
        return recorder->getConfig(configID);
    }

    return 0;
}

/*
 * Class:     com_zayhu_library_jni_OpenSLRecorder
 * Method:    nativeRunEncoding
 * Signature: (ILjava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_OpenSLRecorder_nativeRunEncoding
(JNIEnv * env, jclass clzz, jint nativeObject, jobject javaObject) {
    YeeCallRecorder* recorder = (YeeCallRecorder*) nativeObject;

    if (recorder) {
        return recorder->runEncoding(env, clzz, javaObject);
    }

    return 0;
}

/*
 * Class:     com_zayhu_library_jni_OpenSLRecorder
 * Method:    nativePlayDataAvailable
 * Signature: (I[BII)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_OpenSLRecorder_nativePlayDataAvailable
(JNIEnv * env, jclass clzz, jint nativeObject, jbyteArray data, jint offset, jint len) {
    YeeCallRecorder* recorder = (YeeCallRecorder*) nativeObject;
    int result = 0;

    if (recorder) {
        jboolean isCopy = false;
        jbyte* rawPtr =  env->GetByteArrayElements(data, &isCopy) ;
        result = recorder->playDataAvailable((char*)(rawPtr + offset), len);
        env->ReleaseByteArrayElements(data, rawPtr, JNI_ABORT); // free without copy back
        env->DeleteLocalRef(data);
    }

    return result;
}
