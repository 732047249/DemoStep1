/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_zayhu_library_jni_Echo */

#ifndef _Included_com_zayhu_library_jni_Echo
#define _Included_com_zayhu_library_jni_Echo
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_zayhu_library_jni_Echo
 * Method:    echoinit
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Echo_echoinit
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     com_zayhu_library_jni_Echo
 * Method:    echoplayback
 * Signature: ([S)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Echo_echoplayback
  (JNIEnv *, jobject, jshortArray);

/*
 * Class:     com_zayhu_library_jni_Echo
 * Method:    echocapture
 * Signature: ([S[S)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Echo_echocapture
  (JNIEnv *, jobject, jshortArray, jshortArray);

/*
 * Class:     com_zayhu_library_jni_Echo
 * Method:    echocancellation
 * Signature: ([S[S[S)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Echo_echocancellation
  (JNIEnv *, jobject, jshortArray, jshortArray, jshortArray);

/*
 * Class:     com_zayhu_library_jni_Echo
 * Method:    echoclose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_zayhu_library_jni_Echo_echoclose
  (JNIEnv *, jobject);

/*
 * Class:     com_zayhu_library_jni_Echo
 * Method:    echogetstate
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_com_zayhu_library_jni_Echo_echogetstate
  (JNIEnv *, jobject, jbyteArray);

/*
 * Class:     com_zayhu_library_jni_Echo
 * Method:    echogetstatesize
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_Echo_echogetstatesize
  (JNIEnv *, jobject);

/*
 * Class:     com_zayhu_library_jni_Echo
 * Method:    echorestorestate
 * Signature: ([BI)V
 */
JNIEXPORT void JNICALL Java_com_zayhu_library_jni_Echo_echorestorestate
  (JNIEnv *, jobject, jbyteArray, jint);

#ifdef __cplusplus
}
#endif
#endif
