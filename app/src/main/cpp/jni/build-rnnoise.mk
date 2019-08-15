LOCAL_PATH := $(call my-dir)

############################################################################################################
# Build rnnoise module for android (partial)
############################################################################################################
include $(CLEAR_VARS)

RNNOISE_DIR      := rnnoise

LOCAL_MODULE    := rnnoise

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/$(tools)

LOCAL_SRC_FILES := \
	$(RNNOISE_DIR)/rnnoise.c \
	$(RNNOISE_DIR)/rn_celt_lpc.c \
	$(RNNOISE_DIR)/rn_kiss_fft.c \
	$(RNNOISE_DIR)/rn_pitch.c \
	$(RNNOISE_DIR)/rnn_data.c \
	$(RNNOISE_DIR)/rnn.c


include $(BUILD_STATIC_LIBRARY)

############################################################################################################
# Done
############################################################################################################
