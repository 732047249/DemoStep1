//
//  feedbackDetect.h
//  YeeCall-VoIP
//
//  Created by David on 16/6/27.
//  Copyright © 2016年 YeeCall. All rights reserved.
//

#ifndef __FEEDBACKDETECT_H
#define __FEEDBACKDETECT_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include "OouraFFT.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  FRAME_COUNT_IN_WINDOW  100
#define  FRAME_SIZE_SAMPLERATE_16k  320
#define  LB_SUBBAND_IDX  31  // according to 1kHz
    

typedef enum {
    Normal = 0,
    Nochange = 1,
    Feedback = 2
} FeedbackDetectType;
    
    struct feedbackDetect {
        int frameOffset;
        short *signal_input;
        double *lbr_array;
        double *peak_lbs;
        double *peak_hbs;
        double *den_times;
        double *frame_energy;
        int    *peak_idxs;
        double peak_hb_ltm;
        double peak_lb_ltm;
        double energy_ltm;
        int feedbackAppear;
        int start_counter;
        double pitch_ltm;
        struct OouraFFT *myOouraFFT;
        int    fft_size;
        int    frame_len;
    };

    struct feedbackDetect* nativeHowlingDetectCreat();

    void initialFeedbackDetect(struct feedbackDetect *myFeedbackDetect, int myFs);

    int doFeedbackDetect(short* input, int size, int fdStatus, struct feedbackDetect *myFeedbackDetect);

    void closeFeedbackDetect(struct feedbackDetect *myFeedbackDetect);

#ifdef __cplusplus
}
#endif

#endif
