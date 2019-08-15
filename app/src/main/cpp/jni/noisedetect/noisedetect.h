//
// Created by 杨锐 on 2017/12/5.
//

#ifndef NOISEDETECT_H
#define NOISEDETECT_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  FRAME_NUM  200
#define  FRAME_LENGTH 320
#define  ENVOLOPE_LEN FRAME_NUM * FRAME_LENGTH / 8
#define  BIG_THRETHOLD 0.6
#define  EPS 0.000001
#define  SILENCE_VALUE 200
#define  NOISE_LOW 0.2
#define  NOISE_HIGH 0.7

static float mEnvolope[ENVOLOPE_LEN];
static float mAbs[ENVOLOPE_LEN];
static const float NOISE_RATIO_MIN_DEFAULTS = 0.45f;
static float  mSilenceMax = SILENCE_VALUE;
static float coefB[5] = {4.604273420860672e-09,1.841709368344269e-08,2.762564052516403e-08,1.841709368344269e-08,4.604273420860672e-09};
static float coefA[5] = {1,-3.956716933995412,5.871085054623698,-3.872007538177556,0.957639491217646};

//static FILE *fEnv;
//static FILE *fAbs;
//static FILE *fTmp;

//void initialNoiseDetect( );
//void openfile();
//void closefile();
void filter(const float* x, float* y, int xlen, float* a, float* b, int nfilt);

int doNoiseDetect(short* input, int size, int nFrameCount);

//float getNoiseReduce(short* before, short* after, int size);

float getNoiseRatio(short* input,  int size);

//void closeNoiseDetect( );
#ifdef __cplusplus
}
#endif

#endif