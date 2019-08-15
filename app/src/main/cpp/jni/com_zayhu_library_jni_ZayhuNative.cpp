#include "com_zayhu_library_jni_ZayhuNative.h"

#include <logcat.h>

JNIEXPORT void JNICALL Java_com_zayhu_library_jni_ZayhuNative_nativeSetDebug
  (JNIEnv * env, jclass, jboolean enable) {
  setDebug(enable);
}


