#include <logcat.h>

/*
 * Android log priority values, in ascending priority order.
 */
// typedef enum android_LogPriority {
  // ANDROID_LOG_UNKNOWN = 0,
  // ANDROID_LOG_DEFAULT, /* only for SetMinPriority() */
  // ANDROID_LOG_VERBOSE,
  // ANDROID_LOG_DEBUG,
  // ANDROID_LOG_INFO,
  // ANDROID_LOG_WARN,
  // ANDROID_LOG_ERROR,
  // ANDROID_LOG_FATAL,
  // ANDROID_LOG_SILENT, /* only for SetMinPriority(); must be last */
// } android_LogPriority;

/*
 * Send a formatted string to the log, used like printf(fmt,...)
 */
// int __android_log_print(int prio, const char* tag, const char* fmt, ...)

bool DEBUG = false;

void setDebug(bool enable) {
    DEBUG = enable;
}

bool isDebug() {
   return DEBUG;
}