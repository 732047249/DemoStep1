// ----------------------------------------------------------------------------------------------

#include <jni.h>

#include <string.h>
#include <unistd.h>

#include <speex/speex.h>

#include "logcat.h"

#include "com_zayhu_library_jni_Speex.h"

#include "NativeSpeexCodec.h"

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_Speex
 * Method:    nativeSpeexOpen
 * Signature: (II)I
 */

JNIEXPORT jint JNICALL
Java_com_zayhu_library_jni_Speex_nativeSpeexOpen(JNIEnv * env, jclass cls, jint sampleRate, jint compression) {
    NativeSpeexCodec* nsc = new NativeSpeexCodec();
    nsc->open(sampleRate, compression);
    return (jint) nsc;
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_Speex
 * Method:    nativeSpeexSetQuality
 * Signature: (II)I
 */JNIEXPORT jint JNICALL
Java_com_zayhu_library_jni_Speex_nativeSpeexSetQuality(JNIEnv *env, jclass cls, jint nativeObj, jint compression) {
    NativeSpeexCodec* nsc = (NativeSpeexCodec*) nativeObj;
    return nsc->setQuality(compression);
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_Speex
 * Method:    nativeSpeexGetFrameSize
 * Signature: (I)I
 */JNIEXPORT jint JNICALL
Java_com_zayhu_library_jni_Speex_nativeSpeexGetFrameSize(JNIEnv *env, jclass cls, jint nativeObj) {
    NativeSpeexCodec* nsc = (NativeSpeexCodec*) nativeObj;
    return nsc->getFrameSize();
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_Speex
 * Method:    nativeSpeexEncode
 * Signature: (I[SI[BI)I
 */JNIEXPORT jint JNICALL
Java_com_zayhu_library_jni_Speex_nativeSpeexEncode(JNIEnv *env, jclass cls, jint nativeObj, jshortArray lin,
        jint offset, jbyteArray encoded, jint size) {
    NativeSpeexCodec* nsc = (NativeSpeexCodec*) nativeObj;
    return nsc->encode(env, lin, offset, encoded, size);
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_Speex
 * Method:    nativeSpeexDecode
 * Signature: (I[B[SI)I
 */JNIEXPORT jint JNICALL
Java_com_zayhu_library_jni_Speex_nativeSpeexDecode(JNIEnv *env, jclass cls, jint nativeObj, jbyteArray encoded,
        jshortArray lin, jint size) {
    NativeSpeexCodec* nsc = (NativeSpeexCodec*) nativeObj;
    return nsc->decode(env, encoded, lin, size);
}

// ----------------------------------------------------------------------------------------------

/*
 * Class:     com_zayhu_library_jni_Speex
 * Method:    nativeSpeexClose
 * Signature: (I)V
 */JNIEXPORT void JNICALL
Java_com_zayhu_library_jni_Speex_nativeSpeexClose(JNIEnv *env, jclass cls, jint nativeObj) {
    NativeSpeexCodec* nsc = (NativeSpeexCodec*) nativeObj;
    delete nsc;
}

// ----------------------------------------------------------------------------------------------
