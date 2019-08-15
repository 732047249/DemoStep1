LOCAL_PATH := $(call my-dir)

############################################################################################################
# Build howling module for android (partial)
############################################################################################################
include $(CLEAR_VARS)

HOWLING_DIR      := howling

LOCAL_MODULE    := howling

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/$(HOWLING_DIR) \

LOCAL_SRC_FILES := \
	$(HOWLING_DIR)/fft4g.c \
	$(HOWLING_DIR)/OouraFFT.c \
	$(HOWLING_DIR)/feedbackDetect.c 


include $(BUILD_STATIC_LIBRARY)

############################################################################################################
# Done
############################################################################################################
