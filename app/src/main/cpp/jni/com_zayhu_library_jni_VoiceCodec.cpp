#include <jni.h>
#include "com_zayhu_library_jni_VoiceCodec.h"
#include "NativeVoiceCodec.h"

//------------------------------------------------
#undef com_zayhu_library_jni_VoiceCodec_DEFAULT_COMPRESSION
#define com_zayhu_library_jni_VoiceCodec_DEFAULT_COMPRESSION 3L
//------------------------------------------------
/*
 * Class:     com_zayhu_library_jni_VoiceCodec
 * Method:    nativeOpen
 * Signature: (III)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_VoiceCodec_nativeOpen(JNIEnv *, jclass, jint codec, jint sampleRate,
        jint flags) {
    NativeVoiceCodec* nvc = new NativeVoiceCodec();
    if (nvc->open(codec, sampleRate, flags) == 0) {
        return (jint) nvc;
    }
    else {
        return 0;
    }
}
//------------------------------------------------
/*
 * Class:     com_zayhu_library_jni_VoiceCodec
 * Method:    nativeClose
 * Signature: (I)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_VoiceCodec_nativeClose(JNIEnv *, jclass, jint nativeObject) {
    NativeVoiceCodec* nvc = (NativeVoiceCodec*) nativeObject;

    if (nvc) {
        int result = nvc->close();
        delete nvc;
        return result;
    }

    return -1;

}
//------------------------------------------------
/*
 * Class:     com_zayhu_library_jni_VoiceCodec
 * Method:    nativeDecode
 * Signature: (I[BII[S)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_VoiceCodec_nativeDecode(JNIEnv * env, jclass, jint nativeObject,
        jbyteArray encoded, jint encodedOffset, jint encodedLen, jshortArray decoded, jint decodeFEC) {
    NativeVoiceCodec* nvc = (NativeVoiceCodec*) nativeObject;

    if (nvc) {
        jboolean isCopy = false;

        isCopy = false;
        jbyte* encodedData = encoded ? env->GetByteArrayElements(encoded, &isCopy) : NULL;

        isCopy = false;
        jshort* decodedData = decoded ? env->GetShortArrayElements(decoded, &isCopy) : NULL; // 4096 shorts

        int result = nvc->decode(encodedData + encodedOffset, encodedLen, decodedData, 4096, decodeFEC);

        if (encoded) {
            env->ReleaseByteArrayElements(encoded, encodedData, JNI_ABORT); // free without copy back
        }

        if (decoded) {
            env->ReleaseShortArrayElements(decoded, decodedData, JNI_OK); // free & copy back
        }

        env->DeleteLocalRef(encoded);
        env->DeleteLocalRef(decoded);

        return result;
    }

    return -1;
}
//------------------------------------------------
/*
 * Class:     com_zayhu_library_jni_VoiceCodec
 * Method:    nativeEncode
 * Signature: (I[SI[SIS[B)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_VoiceCodec_nativeEncode(JNIEnv * env, jclass clzz, jint nativeObject,
        jshortArray farEnd, jint farEndOffset, jshortArray nearEnd, jint nearEndOffset, jshort msInSndCardBuf,
        jbyteArray encoded) {
    NativeVoiceCodec* nvc = (NativeVoiceCodec*) nativeObject;

    if (nvc) {
        jboolean isCopy = false;

        isCopy = false;
        jshort* farEndData = farEnd ? env->GetShortArrayElements(farEnd, &isCopy) : NULL;

        isCopy = false;
        jshort* nearEndData = nearEnd ? env->GetShortArrayElements(nearEnd, &isCopy) : NULL;

        isCopy = false;
        jbyte* encodedData = encoded ? env->GetByteArrayElements(encoded, &isCopy) : NULL;

        int result = nvc->encode(env, farEndData + farEndOffset, nearEndData + nearEndOffset, msInSndCardBuf, encodedData,
                4096);

        if (farEnd) {
            env->ReleaseShortArrayElements(farEnd, farEndData, JNI_ABORT); // free without copy back
        }

        if (nearEnd) {
            env->ReleaseShortArrayElements(nearEnd, nearEndData, JNI_OK); // free & copy back
        }

        if (encoded) {
            env->ReleaseByteArrayElements(encoded, encodedData, JNI_OK); // free & copy back
        }

         env->DeleteLocalRef(farEnd);
         env->DeleteLocalRef(nearEnd);
         env->DeleteLocalRef(encoded);

        return result;
    }

    return -1;
}
//------------------------------------------------
/*
 * Class:     com_zayhu_library_jni_VoiceCodec
 * Method:    nativeSetCodecState
 * Signature: (III)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_VoiceCodec_nativeSetCodecState(JNIEnv *, jclass, jint nativeObject,
        jint typeID, jint value) {
    NativeVoiceCodec* nvc = (NativeVoiceCodec*) nativeObject;

    if (nvc) {
        return nvc->setCodecState(typeID, value);
    }

    return -1;
}
//------------------------------------------------
/*
 * Class:     com_zayhu_library_jni_VoiceCodec
 * Method:    nativeGetCodecState
 * Signature: (II)I
 */JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_VoiceCodec_nativeGetCodecState(JNIEnv *, jclass, jint nativeObject,
        jint typeID) {
    NativeVoiceCodec* nvc = (NativeVoiceCodec*) nativeObject;

    if (nvc) {
        return nvc->getCodecState(typeID);
    }

    return -1;

}