LOCAL_PATH := $(call my-dir)
LOCAL_CPPFLAGS += -std=c++11

############################################################################################################
# Build WebRTC for android (partial)
############################################################################################################
include $(CLEAR_VARS)

WEBRTC_DIR      := webrtc-v57

LOCAL_MODULE    := webrtc
LOCAL_CFLAGS    := -DFIXED_POINT -DUSE_KISS_FFT -DEXPORT="" -UHAVE_CONFIG_H -DWEBRTC_ANDROID -DWEBRTC_NS_FIXED -DWEBRTC_APM_DEBUG_DUMP=0 -DWEBRTC_POSIX

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/$(WEBRTC_DIR) \
	$(LOCAL_PATH)/$(WEBRTC_DIR)/webrtc \
	$(LOCAL_PATH)/$(WEBRTC_DIR)/webrtc/base/ \
	$(LOCAL_PATH)/$(WEBRTC_DIR)/webrtc/YC_Interface/ \
	$(LOCAL_PATH)/$(WEBRTC_DIR)/webrtc/common_audio/ \
	$(LOCAL_PATH)/$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/include/ \
	$(LOCAL_PATH)/$(WEBRTC_DIR)/webrtc/modules/audio_processing/utility/       \
	$(LOCAL_PATH)/$(WEBRTC_DIR)/webrtc/modules/audio_processing/aec/           \
	$(LOCAL_PATH)/$(WEBRTC_DIR)/webrtc/modules/audio_processing/aecm/          \
	$(LOCAL_PATH)/$(WEBRTC_DIR)/webrtc/modules/audio_processing/agc/legacy/    \
	$(LOCAL_PATH)/$(WEBRTC_DIR)/webrtc/modules/audio_processing/ns/

LOCAL_WEBRTC_AECM_SRC_FILES := \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/complex_bit_reverse.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/complex_fft.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/copy_set_operations.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/cross_correlation.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/division_operations.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/dot_product_with_scale.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/downsample_fast.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/energy.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/get_scaling_square.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/min_max_operations.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/randomization_functions.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/real_fft.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/spl_init.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/spl_sqrt_floor.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/vector_scaling_operations.c \
	$(WEBRTC_DIR)/webrtc/modules/audio_coding/codecs/ilbc/cb_mem_energy.c \
	$(WEBRTC_DIR)/webrtc/modules/audio_coding/codecs/ilbc/cb_mem_energy_calc.c \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/aecm/aecm_core.cc \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/aecm/aecm_core_c.cc \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/aecm/echo_control_mobile.cc \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/ns/noise_suppression_x.c \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/ns/nsx_core.c \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/ns/nsx_core_c.c \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/utility/block_mean_calculator.cc \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/utility/delay_estimator_wrapper.cc \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/utility/delay_estimator.cc \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/utility/delay_estimator_wrapper.cc \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/utility/ooura_fft.cc \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/agc/legacy/analog_agc.c \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/agc/legacy/digital_agc.c \
	$(WEBRTC_DIR)/webrtc/common_audio/ring_buffer.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/spl_sqrt.c \
	$(WEBRTC_DIR)/webrtc/common_audio/signal_processing/resample_by_2.c \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/logging/apm_data_dumper.cc

LOCAL_WEBRTC_AEC_SRC_FILES := \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/aec/echo_cancellation.cc \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/aec/aec_resampler.cc \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/aec/aec_core.cc

LOCAL_WEBRTC_FILTER_SRC_FILES := \
	$(WEBRTC_DIR)/webrtc/common_audio/fir_filter.cc \
	$(WEBRTC_DIR)/webrtc/system_wrappers/source/aligned_malloc.cc

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_ARM_NEON := true
LOCAL_WEBRTC_FILTER_SRC_FILES += \
	$(WEBRTC_DIR)/webrtc/common_audio/fir_filter_neon.cc
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/utility/ooura_fft_neon.cc \
LOCAL_CFLAGS += -DHAVE_NEON -mfloat-abi=softfp -mfpu=neon -march=armv7-a
endif

ifeq ($(TARGET_ARCH),x86)
LOCAL_WEBRTC_AEC_SRC_FILES += \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/aec/aec_core_sse2.cc \
	$(WEBRTC_DIR)/webrtc/modules/audio_processing/utility/ooura_fft_sse2.cc \
	$(WEBRTC_DIR)/webrtc/system_wrappers/source/cpu_features.cc \
	$(WEBRTC_DIR)/webrtc/common_audio/fir_filter_sse.cc
endif

LOCAL_WEBRTC_BASE_SRC_FILES := \
	$(WEBRTC_DIR)/webrtc/base/checks.cc  \
	$(WEBRTC_DIR)/webrtc/base/criticalsection.cc

LOCAL_WEBRTC_SYSTEMWRAPPER_SRC_FILES := \
	$(WEBRTC_DIR)/webrtc/system_wrappers/source/metrics_default.cc


LOCAL_SRC_FILES := \
	$(LOCAL_WEBRTC_AECM_SRC_FILES) $(LOCAL_WEBRTC_AEC_SRC_FILES) $(LOCAL_WEBRTC_FILTER_SRC_FILES) \
	$(LOCAL_WEBRTC_BASE_SRC_FILES) $(LOCAL_WEBRTC_SYSTEMWRAPPER_SRC_FILES)

include $(BUILD_STATIC_LIBRARY)

############################################################################################################
# Done
############################################################################################################
