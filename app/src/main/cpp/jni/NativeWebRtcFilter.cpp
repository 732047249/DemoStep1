/*
 * NativeWebRtcFilter.cpp
 *
 *  Created on: 2015年4月6日
 *      Author: zhangjianrong
 */

#include <new>
// #include <sstream>

#include "NativeWebRtcFilter.h"
#include "logcat.h"

static const int LOW_PASS_FILTER_ORDER = 33;
static const float LOW_PASS_FILTER_COEFFICIENTS[LOW_PASS_FILTER_ORDER] = {
        0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.031250f,0.000000f,
        -0.031250f,0.031250f,0.062500f,-0.093750f,-0.062500f,0.312500f,0.562500f,0.312500f,-0.062500f,-0.093750f,
        0.062500f,0.031250f,-0.031250f,0.000000f,0.031250f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,
        0.000000f,0.000000f,0.000000f
};

NativeWebRtcFilter::NativeWebRtcFilter()
    :mFilterInputBuffer(NULL), mFilterOutputBuffer(NULL), mBufferSize(0), mFIRFilter(NULL)
{
}

NativeWebRtcFilter::~NativeWebRtcFilter()
{
    this->destroy();
}

bool NativeWebRtcFilter::init(size_t max_input_length)
{
    if (mFIRFilter != NULL) {
        delete mFIRFilter;
        mFIRFilter = NULL;
    }

    mFIRFilter = webrtc::FIRFilter::Create(LOW_PASS_FILTER_COEFFICIENTS, LOW_PASS_FILTER_ORDER, max_input_length);
    if(!mFIRFilter) {
        return false;
    }

    mFilterInputBuffer = new (std::nothrow) float[max_input_length];
    if (!mFilterInputBuffer) {
        destroy();
        return false;
    }

    mFilterOutputBuffer = new (std::nothrow) float[max_input_length];
    if (!mFilterOutputBuffer) {
        destroy();
        return false;
    }

    mBufferSize = max_input_length;

    return true;
}

bool NativeWebRtcFilter::filter(const short* in, size_t length, short* out)
{
    if(mFIRFilter == NULL) {
        return false;
    }

    for(size_t i = 0; i < length && i < mBufferSize; ++i) {
        mFilterInputBuffer[i] = (float)(*(in + i));
    }

    mFIRFilter->Filter(mFilterInputBuffer, length, mFilterOutputBuffer);
    for(size_t i = 0; i < length && i < mBufferSize; ++i) {
        out[i] = (short)(*(mFilterOutputBuffer + i) + 0.5f);
    }

    /*
    std::ostringstream iss, oss;

    iss << "input:";
    for(int i = 0; i < length; ++i) {
        iss << mFilterInputBuffer[i] << " ";
    }

    LOGI("FILTER: %s\n", iss.str().c_str());

    oss << "output:";
    for(int i = 0; i < length; ++i) {
        oss << mFilterOutputBuffer[i] << " ";
    }

    LOGI("FILTER: %s\n", oss.str().c_str());
    */

    return true;
}


void NativeWebRtcFilter::destroy()
{
    if (mFIRFilter != NULL) {
        delete mFIRFilter;
        mFIRFilter = NULL;
    }

    if(mFilterInputBuffer != NULL) {
        delete[] mFilterInputBuffer;
        mFilterInputBuffer = NULL;
    }

    if(mFilterOutputBuffer != NULL) {
        delete[] mFilterOutputBuffer;
        mFilterOutputBuffer = NULL;
    }

    mBufferSize = 0;
}
