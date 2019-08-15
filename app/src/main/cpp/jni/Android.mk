LOCAL_PATH := $(call my-dir)
LOCAL_CPPFLAGS += -std=c++11

############################################################################################################
# Include 3rd party libraries
############################################################################################################

include $(LOCAL_PATH)/build-opus-1.1.3.mk
include $(LOCAL_PATH)/build-speex.mk
include $(LOCAL_PATH)/build-webrtc-v57.mk
include $(LOCAL_PATH)/build-howling.mk
include $(LOCAL_PATH)/build-rnnoise.mk
include $(LOCAL_PATH)/build-noisedetect.mk

############################################################################################################
# Build Zayhu Core Library
############################################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE    := zayhu

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/speex-1.2rc1/include/ \
	$(LOCAL_PATH)/webrtc-v57/ \
	$(LOCAL_PATH)/opus-1.1.3/include/ \
	$(LOCAL_PATH)/howling/ \
	$(LOCAL_PATH)/YCInterface/ \
	$(LOCAL_PATH)/rnnoise/ \
	$(LOCAL_PATH)/noisedetect/

LOCAL_SRC_FILES := \
	ZayhuVoIP.cpp \
	NativeSpeexCodec.cpp \
	NativeOpusCodec.cpp \
	NativeWebRtcAEC.cpp \
	NativeWebRtcAECM.cpp \
	NativeVoiceCodec.cpp \
	NativeWebRtcFilter.cpp \
	HighPassFilter.cpp \
	YeeCallRecorder.cpp \
	\
	YCInterface/IYCWebRtcAEC.cpp \
	YCInterface/IYCWebRtcAECM.cpp \
	YCInterface/IYCWebRtcNSx.cpp \
	YCInterface/IYCWebRtcAgc.cpp \
	\
	CSignal.cpp \
	fft.c \
	latencyMeasurer.cpp \
	\
	com_zayhu_library_jni_Speex.cpp \
	com_zayhu_library_jni_Opus.cpp \
	com_zayhu_library_jni_Echo.cpp \
	com_zayhu_library_jni_WebRTCAEC.cpp \
	com_zayhu_library_jni_WebRTCAECMobile.cpp \
	com_zayhu_library_jni_VoiceCodec.cpp \
	com_zayhu_library_jni_OpenSLRecorder.cpp \
	com_zayhu_library_jni_StatisticUtils.cpp \
	com_zayhu_library_jni_ZayhuNative.cpp \
	\
	logcat.c

LOCAL_STATIC_LIBRARIES := opus speex webrtc howling rnnoise noisedetect

LOCAL_LDLIBS := -lm -llog -lOpenSLES -latomic

include $(BUILD_SHARED_LIBRARY)

############################################################################################################
# Done
############################################################################################################
