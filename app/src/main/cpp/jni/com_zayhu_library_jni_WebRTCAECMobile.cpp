// ----------------------------------------------------------------------------------------------

#include "com_zayhu_library_jni_WebRTCAECMobile.h"

#include "NativeWebRtcAECM.h"

//------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_WebRTCAECMobile
 * Method:    nativeWebRtcAecmInit
 * Signature: (I)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_WebRTCAECMobile_nativeWebRtcAecmInit(JNIEnv *, jclass, jint sampleRate) {
    NativeWebRtcAECM* nwa = new NativeWebRtcAECM();
    nwa->aecmInit(sampleRate);
    return (jint) nwa;
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_WebRTCAECMobile
 * Method:    nativeWebRtcAecmBufferFarend
 * Signature: (I[SS)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_WebRTCAECMobile_nativeWebRtcAecmBufferFarend(JNIEnv * env, jclass,
        jint nativeObject, jshortArray farEnd, jshort nrOfSamples) {
    NativeWebRtcAECM* nwa = (NativeWebRtcAECM*) nativeObject;

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
 * Class:     com_zayhu_library_jni_WebRTCAECMobile
 * Method:    nativeWebRtcAecmProcess
 * Signature: (I[S[SSS)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_WebRTCAECMobile_nativeWebRtcAecmProcess(JNIEnv * env, jclass,
        jint nativeObject, jshortArray nearEndNoisy, jshortArray out, jshort nrOfSamples, jshort msInSndCardBuf) {
    NativeWebRtcAECM* nwa = (NativeWebRtcAECM*) nativeObject;

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
 * Class:     com_zayhu_library_jni_WebRTCAECMobile
 * Method:    nativeWebRtcAecmFree
 * Signature: (I)I
 */JNIEXPORT void JNICALL Java_com_zayhu_library_jni_WebRTCAECMobile_nativeWebRtcAecmFree(JNIEnv *, jclass, jint nativeObject) {
    NativeWebRtcAECM* nwa = (NativeWebRtcAECM*) nativeObject;
    if (nwa) {
        nwa->aecmClose();
        delete nwa;
    }
}

// ----------------------------------------------------------------------------------------------
