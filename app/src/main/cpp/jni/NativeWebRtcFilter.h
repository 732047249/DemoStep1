#ifndef NATIVE_WEBRTC_FILTER_871E5DCCC6528EF4379BA0D2CD255F7C
#define NATIVE_WEBRTC_FILTER_871E5DCCC6528EF4379BA0D2CD255F7C

#include <jni.h>
#include <string.h>
#include <unistd.h>
#include <webrtc/common_audio/fir_filter.h>

#include "logcat.h"

class NativeWebRtcFilter {
public:
    // 
    NativeWebRtcFilter();
    
    // 
    virtual ~NativeWebRtcFilter();
    
    // init
    bool init(size_t max_input_length);

    // filter 
    bool filter(const short * in, size_t length, short* out);
    
    // destory 
    void destroy();
private:
    webrtc::FIRFilter * mFIRFilter;
    float * mFilterInputBuffer;
    float * mFilterOutputBuffer;
    int mBufferSize;
};

#endif
