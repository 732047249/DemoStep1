// ----------------------------------------------------------------------------------------------

#include <jni.h>

#include <string.h>
#include <unistd.h>

#include "logcat.h"

#include "com_zayhu_library_jni_Opus.h"

#include "NativeOpusCodec.h"

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeOpusOpen
 * Signature: (II)I
 */

JNIEXPORT jint JNICALL
Java_com_zayhu_library_jni_Opus_nativeOpusOpen(JNIEnv * env, jclass cls, jint sampleRate, jint compression,
        jint flags) {
    NativeOpusCodec* noc = new NativeOpusCodec();
    noc->open(sampleRate, compression, flags);
    return (jint) noc;
}

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeOpusSetQuality
 * Signature: (II)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Opus_nativeOpusSetQuality(JNIEnv *, jclass, jint nativeObj,
        jint compression) {
    NativeOpusCodec* noc = (NativeOpusCodec*) nativeObj;
    return noc->setQuality(compression);
}

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeOpusSetGain
 * Signature: (II)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Opus_nativeOpusSetGain(JNIEnv *, jclass, jint nativeObj, jint gain) {
    NativeOpusCodec* noc = (NativeOpusCodec*) nativeObj;
    return noc->setGain(gain);
}

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeOpusSetAGC
 * Signature: (II)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Opus_nativeOpusSetAGC(JNIEnv *, jclass, jint nativeObj,
        jint initialNormal) {
    NativeOpusCodec* noc = (NativeOpusCodec*) nativeObj;
    return noc->setAGC(initialNormal);
}

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeOpusGetAGC
 * Signature: (I)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Opus_nativeOpusGetAGC(JNIEnv *, jclass, jint nativeObj) {
    NativeOpusCodec* noc = (NativeOpusCodec*) nativeObj;
    return noc->getAGC();
}

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeOpusSetDcOffset
 * Signature: (II)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Opus_nativeOpusSetDcOffset(JNIEnv *, jclass, jint nativeObj,
        jint initialOffset) {
    NativeOpusCodec* noc = (NativeOpusCodec*) nativeObj;
    return noc->setDcOffset(initialOffset);
}

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeOpusGetDcOffset
 * Signature: (I)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Opus_nativeOpusGetDcOffset(JNIEnv *, jclass, jint nativeObj) {
    NativeOpusCodec* noc = (NativeOpusCodec*) nativeObj;
    return noc->getDcOffset();
}

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeSetExpectedLossRate
 * Signature: (II)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Opus_nativeSetExpectedLossRate(JNIEnv *, jclass, jint nativeObj,
        jint percent) {
    NativeOpusCodec* noc = (NativeOpusCodec*) nativeObj;
    return noc->setExpectedLossRate(percent);
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeOpusGetFrameSize
 * Signature: (I)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Opus_nativeOpusGetFrameSize(JNIEnv *, jclass, jint nativeObj) {
    NativeOpusCodec* noc = (NativeOpusCodec*) nativeObj;
    return noc->getFrameSize();
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeOpusEncode
 * Signature: (I[SI[BI)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Opus_nativeOpusEncode(JNIEnv * env, jclass, jint nativeObj,
        jshortArray lin, jint offset, jbyteArray encoded, jint size) {
    NativeOpusCodec* noc = (NativeOpusCodec*) nativeObj;
    return noc->encode(env, lin, offset, encoded, size);
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeOpusDecode
 * Signature: (I[B[SI)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Opus_nativeOpusDecode(JNIEnv * env, jclass, jint nativeObj,
        jbyteArray encoded, jshortArray lin, jint size) {
    NativeOpusCodec* noc = (NativeOpusCodec*) nativeObj;
    return noc->decode(env, encoded, lin, size);
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_Opus
 * Method:    nativeOpusClose
 * Signature: (I)V
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Opus_nativeOpusClose(JNIEnv *, jclass, jint nativeObj) {
    NativeOpusCodec* noc = (NativeOpusCodec*) nativeObj;

    if (noc) {
        int result = noc->close();
        delete noc;
        return result;
    }

    return -1;
}
// ----------------------------------------------------------------------------------------------
