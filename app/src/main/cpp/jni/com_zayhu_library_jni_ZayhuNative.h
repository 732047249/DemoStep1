#include <jni.h>
#include <stdlib.h>

#ifndef _Included_com_zayhu_library_jni_ZayhuNative
#define _Included_com_zayhu_library_jni_ZayhuNative
#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void JNICALL Java_com_zayhu_library_jni_ZayhuNative_nativeSetDebug
  (JNIEnv *, jclass, jboolean);

#ifdef __cplusplus
}
#endif
#endif
