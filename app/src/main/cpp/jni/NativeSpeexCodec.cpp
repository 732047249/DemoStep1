// ----------------------------------------------------------------------------------------------

#include "NativeSpeexCodec.h"

// ----------------------------------------------------------------------------------------------

NativeSpeexCodec::NativeSpeexCodec()
        : mSampleRate(0), mCompression(0), mOpenRef(0), mFrameSizeDecode(0), mFrameSizeEncode(0), mStateEncode(NULL), mStateDecode(
                NULL) {
    memset(&mSpeexBitsEncode, 0, sizeof(mSpeexBitsEncode));
    memset(&mSpeexBitsDecode, 0, sizeof(mSpeexBitsDecode));
}

NativeSpeexCodec::~NativeSpeexCodec() {
}

// ----------------------------------------------------------------------------------------------

int NativeSpeexCodec::open(int sampleRate, int compression) {
    LOGI("open: sampleRate=%d, compression=%d", sampleRate, compression);

    int ret = 0;
    int tmp = 0;

    mOpenRef++;

    // save
    mSampleRate = sampleRate;
    mCompression = compression;

    // setup bits
    speex_bits_init(&mSpeexBitsEncode);
    speex_bits_init(&mSpeexBitsDecode);

    // setup sample rate
    if (sampleRate == 32000) {
        mStateEncode = speex_encoder_init(&speex_uwb_mode);
        mStateDecode = speex_decoder_init(&speex_uwb_mode);
    }
    else if (sampleRate == 16000) {
        mStateEncode = speex_encoder_init(&speex_wb_mode);
        mStateDecode = speex_decoder_init(&speex_wb_mode);
    }
    else if (sampleRate == 8000) {
        mStateEncode = speex_encoder_init(&speex_nb_mode);
        mStateDecode = speex_decoder_init(&speex_nb_mode);
    }
    else {
        return -1;
    }

    tmp = sampleRate;
    ret = speex_decoder_ctl(mStateDecode, SPEEX_SET_SAMPLING_RATE, &tmp);
    if (ret != 0) {
        LOGI("Error: could not set codec sample rate: %d", ret);
    }

    // get frame size on open
    speex_encoder_ctl(mStateEncode, SPEEX_GET_FRAME_SIZE, &mFrameSizeEncode);
    speex_decoder_ctl(mStateDecode, SPEEX_GET_FRAME_SIZE, &mFrameSizeDecode);

    LOGI("Init speex - framesize: enc=%d, dec=%d, sampleRate=%d", mFrameSizeEncode, mFrameSizeDecode, sampleRate);

    int vbr = 1;
    speex_encoder_ctl(mStateEncode, SPEEX_SET_VBR, &vbr);

    int vad = 1;
    speex_encoder_ctl(mStateEncode, SPEEX_SET_VAD, &vad);

    int dtx = 1;
    speex_encoder_ctl(mStateEncode, SPEEX_SET_DTX, &dtx);

    int enh = 1;
    speex_decoder_ctl(mStateDecode, SPEEX_SET_ENH, &enh);

    int plc = 10;
    speex_encoder_ctl(mStateEncode, SPEEX_SET_PLC_TUNING, &plc);

    /*
     int highpass = 1;
     speex_encoder_ctl(enc_state, SPEEX_SET_HIGHPASS, &highpass );
     speex_decoder_ctl(dec_state, SPEEX_SET_HIGHPASS, &highpass );

     int complexity = 4;
     speex_encoder_ctl(enc_state, SPEEX_SET_COMPLEXITY, &complexity );
     */

    // finally, setup compression
    setQuality(compression);

    return 0;
}

void NativeSpeexCodec::close() {
    if (!mOpenRef) {
        LOGE("Error: codec not opened.");
        return;
    }

    mOpenRef--;

    speex_bits_destroy(&mSpeexBitsEncode);
    speex_bits_destroy(&mSpeexBitsDecode);
    speex_decoder_destroy(mStateDecode);
    speex_encoder_destroy(mStateEncode);
}

int NativeSpeexCodec::setQuality(int compression) {
    if (!mOpenRef) {
        LOGE("Error: codec not opened.");
        return mCompression;
    }

    int old = mCompression;
    mCompression = compression;

    int tmp = compression;
    speex_encoder_ctl(mStateEncode, SPEEX_SET_QUALITY, &tmp);

    float quality = tmp;
    speex_encoder_ctl(mStateEncode, SPEEX_SET_VBR_QUALITY, &quality);
    return old;
}

int NativeSpeexCodec::getFrameSize() {
    if (!mOpenRef) {
        LOGE("Error: codec not opened.");
        return -1;
    }

    return mFrameSizeEncode;
}

// ----------------------------------------------------------------------------------------------

int NativeSpeexCodec::encode(JNIEnv *env, jshortArray lin, jint offset, jbyteArray encoded, jint size) {
    if (!mOpenRef) {
        LOGE("Error: codec not opened.");
        return -1;
    }

    jshort buffer[mFrameSizeEncode];
    jbyte output_buffer[mFrameSizeEncode];

    int nsamples = (size - 1) / mFrameSizeEncode + 1;
    int i, tot_bytes = 0;

    speex_bits_reset(&mSpeexBitsEncode);

    for (i = 0; i < nsamples; i++) {
        env->GetShortArrayRegion(lin, offset + i * mFrameSizeEncode, mFrameSizeEncode, buffer);
        speex_encode_int(mStateEncode, buffer, &mSpeexBitsEncode);
    }

    tot_bytes = speex_bits_write(&mSpeexBitsEncode, (char *) output_buffer, mFrameSizeEncode);

    env->SetByteArrayRegion(encoded, 0, tot_bytes, output_buffer);

    // LOGI("Encoding: %d audio, %d frames, enc_frame_size: %d, total: %d", size, nsamples, enc_frame_size, tot_bytes);

    return (jint) tot_bytes;
}

int NativeSpeexCodec::decode(JNIEnv *env, jbyteArray encoded, jshortArray lin, jint size) {
    if (!mOpenRef) {
        LOGE("Error: codec not opened.");
        return -1;
    }

    jbyte buffer[mFrameSizeDecode];
    jshort output_buffer[mFrameSizeDecode];
    jsize encoded_length = size;

    if (encoded == 0 || size == 0) {
        speex_decode_int(mStateDecode, 0, output_buffer);
        env->SetShortArrayRegion(lin, 0, mFrameSizeDecode, output_buffer);
    }
    else {
        env->GetByteArrayRegion(encoded, 0, encoded_length, buffer);

        speex_bits_read_from(&mSpeexBitsDecode, (char *) buffer, encoded_length);
        speex_decode_int(mStateDecode, &mSpeexBitsDecode, output_buffer);

        env->SetShortArrayRegion(lin, 0, mFrameSizeDecode, output_buffer);
    }

    return (jint) mFrameSizeDecode;
}

// ----------------------------------------------------------------------------------------------
