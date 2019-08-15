// ----------------------------------------------------------------------------------------------

#include <jni.h>
#include <string.h>
#include <unistd.h>
#include <speex/speex_echo.h> //used for echo cancellation
#include <speex/speex_preprocess.h> //used for preprocessor
#include "logcat.h"
#include "com_zayhu_library_jni_Echo.h"

// ----------------------------------------------------------------------------------------------

SpeexEchoState *echostate;
SpeexEchoStateBlob *echostateblob;
static int echostatesize;

SpeexPreprocessState *preprocess_state;
static int eco_frame_size;
static int eco_open = 0;

// ----------------------------------------------------------------------------------------------

extern "C" JNIEXPORT jint JNICALL
Java_com_zayhu_library_jni_Echo_echoinit(JNIEnv *env, jobject obj, jint framesize, jint filterlength) {
    if (eco_open++ != 0) return (jint) 0;

    echostate = speex_echo_state_init(framesize, filterlength);
    jint sampleRate = 8000;
    speex_echo_ctl(echostate, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);
    preprocess_state = speex_preprocess_state_init(framesize, sampleRate);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_ECHO_STATE, echostate);

    jint denoise = 1;
    jint noiseSuppress = -30;
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise); //denoise
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress); //noise dB

    jint echoSuppress = -45;
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, &echoSuppress);
    echoSuppress = -15;
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE, &echoSuppress);

    /*
     jint vad = 1;
     jint vadProbStart = 80;
     jint vadProbContinue = 65;
     speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_VAD, &vad); //silence detect
     speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_PROB_START , &vadProbStart); //Set probability required for the VAD to go from silence to voice
     speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &vadProbContinue); //Set probability required for the VAD to stay in the voice state (integer percent)
     */

    jint agc = 1;
    jfloat q = 12000.0f;
    jint gain = 15;
    //actually default is 8000(0,32768),here make it louder for voice is not loudy enough by default. 8000

    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC, &agc); //����
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &q);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &gain);

    eco_frame_size = framesize;
    if (echostate == 0) return (jint) 0;
    else return (jint) 1;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_zayhu_library_jni_Echo_echoplayback(JNIEnv *env, jobject obj, jshortArray play) {
    if (!eco_open) return 0;
    jshort temp_buffer[eco_frame_size];
    env->GetShortArrayRegion(play, 0, eco_frame_size, temp_buffer);
    speex_echo_playback(echostate, temp_buffer);
    return (jint) eco_frame_size;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_zayhu_library_jni_Echo_echocapture(JNIEnv *env, jobject obj, jshortArray rec, jshortArray out) {
    if (!eco_open) return 0;
    jshort temp_buffer[eco_frame_size];
    jshort temp_out[eco_frame_size];
    env->GetShortArrayRegion(rec, 0, eco_frame_size, temp_buffer);
    jint speech = speex_preprocess_run(preprocess_state, temp_buffer);
    speex_echo_capture(echostate, temp_buffer, temp_out);
    env->SetShortArrayRegion(out, 0, eco_frame_size, temp_out);
    return (jint) speech;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_zayhu_library_jni_Echo_echocancellation(JNIEnv *env, jobject obj, jshortArray input, jshortArray echo,
                jshortArray out) {
    if (!eco_open) return 0;
    jshort temp_input[eco_frame_size];
    jshort temp_echo[eco_frame_size];
    jshort temp_out[eco_frame_size];
    env->GetShortArrayRegion(input, 0, eco_frame_size, temp_input);
    env->GetShortArrayRegion(echo, 0, eco_frame_size, temp_echo);
    //jint speech = speex_preprocess_run(preprocess_state, temp_input);
    speex_echo_cancellation(echostate, temp_input, temp_echo, temp_out);
    jint speech = speex_preprocess_run(preprocess_state, temp_out);
    env->SetShortArrayRegion(out, 0, eco_frame_size, temp_out);
    //jint speech = 1;
    return (jint) speech;
}

extern "C" JNIEXPORT void JNICALL
Java_com_zayhu_library_jni_Echo_echoclose(JNIEnv *env, jobject obj) {
    if (--eco_open != 0) return;
    speex_echo_state_destroy(echostate);
    speex_preprocess_state_destroy(preprocess_state);
}

extern "C" JNIEXPORT void JNICALL
Java_com_zayhu_library_jni_Echo_echogetstate(JNIEnv *env, jobject obj, jbyteArray state) {
    if (!eco_open) return;
    jbyte *echostatebyte;
    echostatebyte = speex_echo_state_blob_get_data(echostateblob);
    env->SetByteArrayRegion(state, 0, echostatesize, echostatebyte);
    speex_echo_state_blob_free(echostateblob);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_zayhu_library_jni_Echo_echogetstatesize(JNIEnv *env, jobject obj) {
    if (!eco_open) return 0;
    speex_echo_ctl(echostate, SPEEX_ECHO_GET_BLOB, &echostateblob);
    echostatesize = speex_echo_state_blob_get_size(echostateblob);
    return (jint) echostatesize;
}

extern "C" JNIEXPORT void JNICALL
Java_com_zayhu_library_jni_Echo_echorestorestate(JNIEnv *env, jobject obj, jbyteArray state, jint size) {
    if (!eco_open) return;
    jbyte temp[size];
    env->GetByteArrayRegion(state, 0, size, temp);
    echostateblob = speex_echo_state_blob_new_from_memory(temp, size);
    speex_echo_ctl(echostate, SPEEX_ECHO_SET_BLOB, echostateblob);
}
