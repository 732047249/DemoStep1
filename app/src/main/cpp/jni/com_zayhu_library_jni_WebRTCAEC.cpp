// ----------------------------------------------------------------------------------------------

#include "com_zayhu_library_jni_WebRTCAEC.h"

#include "NativeWebRtcAEC.h"

//------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_WebRTCAEC
 * Method:    nativeWebRtcAecmInit
 * Signature: (I)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_WebRTCAEC_nativeWebRtcAecmInit(JNIEnv *, jclass, jint sampleRate) {
    NativeWebRtcAEC* nwa = new NativeWebRtcAEC();
    nwa->aecmInit(sampleRate);
    return (jint) nwa;
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_WebRTCAEC
 * Method:    nativeWebRtcAecmBufferFarend
 * Signature: (I[SS)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_WebRTCAEC_nativeWebRtcAecmBufferFarend(JNIEnv * env, jclass,
        jint nativeObject, jshortArray farEnd, jshort nrOfSamples) {
    NativeWebRtcAEC* nwa = (NativeWebRtcAEC*) nativeObject;

    if (nwa) {
        jboolean isCopy = false;
        jshort* farEndData = env->GetShortArrayElements(farEnd, &isCopy);

        int result = nwa->aecmBufferFarEnd(farEndData, nrOfSamples);

        env->ReleaseShortArrayElements(farEnd, farEndData, JNI_ABORT); // free without copy back

        env->DeleteLocalRef(farEnd);
        return result;
    }

    return -1;
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_WebRTCAEC
 * Method:    nativeWebRtcAecmProcess
 * Signature: (I[S[SSS)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_WebRTCAEC_nativeWebRtcAecmProcess(JNIEnv * env, jclass,
        jint nativeObject, jshortArray nearEndNoisy, jshortArray out, jshort nrOfSamples, jshort msInSndCardBuf) {
    NativeWebRtcAEC* nwa = (NativeWebRtcAEC*) nativeObject;

    if (nwa) {
        jboolean isCopy = false;
        jshort* nearEndNoisyData = env->GetShortArrayElements(nearEndNoisy, &isCopy);

        isCopy = false;
        jshort* outData = env->GetShortArrayElements(out, &isCopy);

        int result = nwa->aecmProcess(nearEndNoisyData, outData, nrOfSamples, msInSndCardBuf);

        env->ReleaseShortArrayElements(out, outData, 0); // free & copy back
        env->ReleaseShortArrayElements(nearEndNoisy, nearEndNoisyData, JNI_ABORT); // free without copy back

        env->DeleteLocalRef(nearEndNoisy);
        env->DeleteLocalRef(out);
        return result;
    }

    return -1;
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_WebRTCAEC
 * Method:    nativeWebRtcAecmFree
 * Signature: (I)I
 */JNIEXPORT void JNICALL Java_com_zayhu_library_jni_WebRTCAEC_nativeWebRtcAecmFree(JNIEnv *, jclass, jint nativeObject) {
    NativeWebRtcAEC* nwa = (NativeWebRtcAEC*) nativeObject;
    if (nwa) {
        nwa->aecmClose();
        delete nwa;
    }
}

// ----------------------------------------------------------------------------------------------
