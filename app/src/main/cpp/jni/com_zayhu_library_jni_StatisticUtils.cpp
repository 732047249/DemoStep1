#include "com_zayhu_library_jni_StatisticUtils.h"

#include <algorithm>

/*
 * Class:     com_zayhu_library_jni_StatisticUtils
 * Method:    nativeFindNthMimimal
 * Signature: ([IIII)I
 */
JNIEXPORT jint JNICALL Java_com_zayhu_library_jni_StatisticUtils_nativeFindNthMinimal
  (JNIEnv * env, jclass, jintArray data, jint offset, jint length, jint pos) {
    jboolean isCopy = false;

    isCopy = false;
    jint* dataRawOrig = env->GetIntArrayElements(data, &isCopy);
    jint* dataRaw = dataRawOrig + offset;

    std::nth_element<jint*>(dataRaw, dataRaw + pos, dataRaw + length);
    int result = *(dataRaw + pos);

    env->ReleaseIntArrayElements(data, dataRawOrig, JNI_ABORT); // free without copy back
    env->DeleteLocalRef(data);

    return result;
}


