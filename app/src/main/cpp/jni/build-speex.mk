LOCAL_PATH := $(call my-dir)

############################################################################################################
# Build Speex for android
############################################################################################################
include $(CLEAR_VARS)

SPPEX_DIR       := speex-1.2rc1

LOCAL_MODULE    := speex
LOCAL_CFLAGS    := -DFIXED_POINT -DUSE_KISS_FFT -DEXPORT="" -UHAVE_CONFIG_H

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/$(SPPEX_DIR)/include

LOCAL_SPEEX_SRC_FILES := \
	$(SPPEX_DIR)/libspeex/bits.c \
	$(SPPEX_DIR)/libspeex/buffer.c \
	$(SPPEX_DIR)/libspeex/cb_search.c \
	$(SPPEX_DIR)/libspeex/exc_10_16_table.c \
	$(SPPEX_DIR)/libspeex/exc_10_32_table.c \
	$(SPPEX_DIR)/libspeex/exc_20_32_table.c \
	$(SPPEX_DIR)/libspeex/exc_5_256_table.c \
	$(SPPEX_DIR)/libspeex/exc_5_64_table.c \
	$(SPPEX_DIR)/libspeex/exc_8_128_table.c \
	$(SPPEX_DIR)/libspeex/fftwrap.c \
	$(SPPEX_DIR)/libspeex/filterbank.c \
	$(SPPEX_DIR)/libspeex/filters.c \
	$(SPPEX_DIR)/libspeex/gain_table.c \
	$(SPPEX_DIR)/libspeex/gain_table_lbr.c \
	$(SPPEX_DIR)/libspeex/hexc_10_32_table.c \
	$(SPPEX_DIR)/libspeex/hexc_table.c \
	$(SPPEX_DIR)/libspeex/high_lsp_tables.c \
	$(SPPEX_DIR)/libspeex/jitter.c \
	$(SPPEX_DIR)/libspeex/kiss_fft.c \
	$(SPPEX_DIR)/libspeex/kiss_fftr.c \
	$(SPPEX_DIR)/libspeex/lpc.c \
	$(SPPEX_DIR)/libspeex/lsp.c \
	$(SPPEX_DIR)/libspeex/lsp_tables_nb.c \
	$(SPPEX_DIR)/libspeex/ltp.c \
	$(SPPEX_DIR)/libspeex/mdf.c \
	$(SPPEX_DIR)/libspeex/modes.c \
	$(SPPEX_DIR)/libspeex/modes_wb.c \
	$(SPPEX_DIR)/libspeex/nb_celp.c \
	$(SPPEX_DIR)/libspeex/preprocess.c \
	$(SPPEX_DIR)/libspeex/quant_lsp.c \
	$(SPPEX_DIR)/libspeex/resample.c \
	$(SPPEX_DIR)/libspeex/sb_celp.c \
	$(SPPEX_DIR)/libspeex/scal.c \
	$(SPPEX_DIR)/libspeex/smallft.c \
	$(SPPEX_DIR)/libspeex/speex.c \
	$(SPPEX_DIR)/libspeex/speex_callbacks.c \
	$(SPPEX_DIR)/libspeex/speex_header.c \
	$(SPPEX_DIR)/libspeex/stereo.c \
	$(SPPEX_DIR)/libspeex/vbr.c \
	$(SPPEX_DIR)/libspeex/vq.c \
	$(SPPEX_DIR)/libspeex/window.c

LOCAL_SRC_FILES := \
	$(LOCAL_SPEEX_SRC_FILES)

include $(BUILD_STATIC_LIBRARY)

############################################################################################################
# Done
############################################################################################################
