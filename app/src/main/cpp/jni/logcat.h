#ifndef LOGCAT_H
#define LOGCAT_H

#include <android/log.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void setDebug(bool enable);
bool isDebug();

#if 0
#define LOGI(...) if (isDebug()) __android_log_print(ANDROID_LOG_INFO   , ("[YC]ZJNI"), __VA_ARGS__)
#define LOGV(...) if (isDebug()) __android_log_print(ANDROID_LOG_VERBOSE, ("[YC]ZJNI"), __VA_ARGS__)
#define LOGD(...) if (isDebug()) __android_log_print(ANDROID_LOG_DEBUG  , ("[YC]ZJNI"), __VA_ARGS__)
#define LOGW(...) if (isDebug()) __android_log_print(ANDROID_LOG_WARN   , ("[YC]ZJNI"), __VA_ARGS__)

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , ("[YC]ZJNI"), __VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

#endif