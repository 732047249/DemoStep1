#include <jni.h>
#include "logcat.h"

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
		return -1;
	}

	// Get jclass with env->FindClass.
	// Register methods with env->RegisterNatives.

	LOGI("loading jni success.");

	return JNI_VERSION_1_6;
}
