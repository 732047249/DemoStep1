//
//  feedbackDetect.c
//  YeeCall-VoIP
//
//  Created by David on 16/8/5.
//  Copyright © 2016年 YeeCall. All rights reserved.
//

#include "feedbackDetect.h"
#include "logcat.h"

struct feedbackDetect* nativeHowlingDetectCreat()
{
    struct feedbackDetect *ptDetect = malloc(sizeof(struct feedbackDetect));
    return  ptDetect;
}

void initialFeedbackDetect(struct feedbackDetect *myFeedbackDetect, int myFs)
{
    if (myFs == 8000) {
        myFeedbackDetect->fft_size = 256;
        myFeedbackDetect->frame_len = 160;
//        LOGI("Howling: Fs = 8000");
    } else {
        myFeedbackDetect->fft_size = 512;
        myFeedbackDetect->frame_len = 320;
    }

    myFeedbackDetect->lbr_array = (double *)malloc(FRAME_COUNT_IN_WINDOW * sizeof(double));

    myFeedbackDetect->peak_lbs = (double *)malloc(FRAME_COUNT_IN_WINDOW * sizeof(double));

    myFeedbackDetect->peak_hbs = (double *)malloc(FRAME_COUNT_IN_WINDOW * sizeof(double));
    
    myFeedbackDetect->den_times = (double *)malloc(FRAME_COUNT_IN_WINDOW * sizeof(double));
    
    myFeedbackDetect->peak_idxs = (int *)malloc(FRAME_COUNT_IN_WINDOW * sizeof(int));
    
    myFeedbackDetect->signal_input = (short *)malloc((myFeedbackDetect->frame_len<<1) * sizeof(short));
    
    myFeedbackDetect->frame_energy = (double *)malloc(FRAME_COUNT_IN_WINDOW * sizeof(double));

    myFeedbackDetect->myOouraFFT = (struct OouraFFT *)malloc(sizeof(struct OouraFFT));

    if(myFeedbackDetect->myOouraFFT) {
        initForSignalsOfLength(myFeedbackDetect->myOouraFFT, myFeedbackDetect->fft_size, kWindow_Hamming);
    }

    myFeedbackDetect->frameOffset = 0;
//    myFeedbackDetect->energy_lb = 0.0f;
//    myFeedbackDetect->energy_hb = 0.0f;
    memset(myFeedbackDetect->lbr_array, 0, FRAME_COUNT_IN_WINDOW*sizeof(double));
    memset(myFeedbackDetect->peak_lbs, 0, FRAME_COUNT_IN_WINDOW*sizeof(double));
    memset(myFeedbackDetect->peak_hbs, 0, FRAME_COUNT_IN_WINDOW*sizeof(double));
    memset(myFeedbackDetect->den_times, 0, FRAME_COUNT_IN_WINDOW*sizeof(double));
    memset(myFeedbackDetect->peak_idxs, 0, FRAME_COUNT_IN_WINDOW*sizeof(int));
    memset(myFeedbackDetect->frame_energy, 0, FRAME_COUNT_IN_WINDOW*sizeof(double));
    memset(myFeedbackDetect->signal_input, 0, (myFeedbackDetect->frame_len<<1)*sizeof(short));
    
    myFeedbackDetect->peak_hb_ltm = 0.0f;
    myFeedbackDetect->peak_lb_ltm = 0.0f;
    myFeedbackDetect->energy_ltm = 0.0f;
    myFeedbackDetect->start_counter = 0;

}

int doFeedbackDetect(short* input, int size, int fdStatus, struct feedbackDetect *myFeedbackDetect)
{
//    if (size != FRAME_SIZE_SAMPLERATE_16k) {
//        return Nochange;
//    }
    
    if (size != myFeedbackDetect->frame_len) {
        return Nochange;
    }
    
    long long mag = 0;
    int  i, j, k;
    for (i = 0; i < size; i ++) {
        mag += abs(input[i]);
    }
    if (mag == 0) {
//        LOGI("Howling: input energy = 0");
        return Nochange;
    }

    memcpy(myFeedbackDetect->signal_input+myFeedbackDetect->frameOffset, input, myFeedbackDetect->frame_len*sizeof(short));

    if (myFeedbackDetect->frameOffset > 0) {
        for (i = 0; i < (myFeedbackDetect->fft_size - myFeedbackDetect->frame_len); i ++) {
            myFeedbackDetect->myOouraFFT->inputData[i] = (double)myFeedbackDetect->signal_input[i + myFeedbackDetect->frameOffset - (myFeedbackDetect->fft_size - myFeedbackDetect->frame_len)];
        }
    }
    for (i = 0; i < myFeedbackDetect->frame_len; i ++) {
        myFeedbackDetect->myOouraFFT->inputData[i + (myFeedbackDetect->fft_size - myFeedbackDetect->frame_len)] = (double)myFeedbackDetect->signal_input[i + myFeedbackDetect->frameOffset];
    }

    calculateOourFFT(myFeedbackDetect->myOouraFFT, 1);

    double energy_fb = 0.0f;
    double energy_lb = 0.0f;
    double peak_lb = 0;
    for (k = 1; k < LB_SUBBAND_IDX; k ++) {
        energy_lb += myFeedbackDetect->myOouraFFT->spectrumData[k];
        peak_lb = (peak_lb < myFeedbackDetect->myOouraFFT->spectrumData[k]) ? myFeedbackDetect->myOouraFFT->spectrumData[k] : peak_lb;
    }
    //            energy_fb += energy_lb;
    double peak_hb = 0;
    int peak_index_hb = 0;
    double peak_density = 0.0f;
    double nonPeak_desity = 0.0f;
    
    for (k = LB_SUBBAND_IDX; k < (myFeedbackDetect->fft_size>>1); k ++) {
        energy_fb += myFeedbackDetect->myOouraFFT->spectrumData[k];
        if (peak_hb < myFeedbackDetect->myOouraFFT->spectrumData[k]) {
            peak_index_hb = k;
            peak_hb = myFeedbackDetect->myOouraFFT->spectrumData[k] ;
        }
    }
    for (i = -1; i < 2; i ++) {
        peak_density += myFeedbackDetect->myOouraFFT->spectrumData[peak_index_hb + i] ;
    }
    nonPeak_desity = (energy_fb - peak_density) / ((myFeedbackDetect->fft_size>>1) - (LB_SUBBAND_IDX+3));
    peak_density = peak_density / 3;
    
    double lbr = (energy_fb) / (energy_fb + energy_lb + 1.0f);
    double valid_energy_est;
    
    valid_energy_est = energy_fb + energy_lb - peak_density*3;
    
    if (myFeedbackDetect->start_counter == 0) {
        myFeedbackDetect->energy_ltm = energy_fb + energy_lb;
        myFeedbackDetect->pitch_ltm = peak_lb;
    }
    
    double peak = peak_hb > peak_lb ? peak_hb : peak_lb;
    if (peak < 0.5 * myFeedbackDetect->pitch_ltm) {
        peak = (peak < 0.5 * myFeedbackDetect->pitch_ltm) ? 0.5 * myFeedbackDetect->pitch_ltm : peak;
    }

    short *grads = malloc((myFeedbackDetect->fft_size>>1)*sizeof(short));
    //    memset(grads, 0, (YC_FFT_SIZE>>1)*sizeof(short));
    for (i = 0; i < (myFeedbackDetect->fft_size>>1); i ++) {
        if (myFeedbackDetect->myOouraFFT->spectrumData[i] > 0.15 * peak) {
            if (myFeedbackDetect->myOouraFFT->spectrumData[i] > myFeedbackDetect->myOouraFFT->spectrumData[i - 1]) {
                if (i > 0) {
                    grads[i - 1] = 0;
                }
                grads[i] = 1;
            } else {
                grads[i] = 0;
            }
        } else {
            grads[i] = 0;
            if ((i < (myFeedbackDetect->fft_size>>1) - 1) && (i > 0)) {
                if((myFeedbackDetect->myOouraFFT->spectrumData[i-1] > 0.15 * peak) && (myFeedbackDetect->myOouraFFT->spectrumData[i+1] > 0.15 * peak)) {
                    grads[i] = 1;
                } else  if ((i < (myFeedbackDetect->fft_size>>1) - 1) && (i > 1)) {
                    if((myFeedbackDetect->myOouraFFT->spectrumData[i-2] > 0.15 * peak) && (myFeedbackDetect->myOouraFFT->spectrumData[i+1] > 0.15 * peak)) {
                        grads[i] = 1;
                    }
                } else  if ((i < (myFeedbackDetect->fft_size>>1) - 2) && (i > 0)) {
                    if((myFeedbackDetect->myOouraFFT->spectrumData[i-1] > 0.15 * peak) && (myFeedbackDetect->myOouraFFT->spectrumData[i+2] > 0.15 * peak)) {
                        grads[i] = 1;
                    }
                }
                
            }
        }
    }

    for (i = 0; i < (myFeedbackDetect->fft_size>>1);) {
        int stop = ((i + 4) < (myFeedbackDetect->fft_size>>1)) ? i + 4 : (myFeedbackDetect->fft_size>>1) - 1;
        int sum = 0;
        int sum_idx = 0;
        for (j = i; j <= stop; j ++)
        {
            if (grads[j] > 0) {
                sum += grads[j];
                sum_idx += j;
            }
        }
        if (sum > 1) {
            for (j = i; j <= stop; j ++)
            {
                grads[j] = 0;
            }
            i = sum_idx/sum - 1;
            if (i < (myFeedbackDetect->fft_size>>1) - 1) {
                grads[i+1] = 1;
            }
        }
        i ++;
    }
    int harmonic_counter = 0;
    for (i = 0; i < (myFeedbackDetect->fft_size>>1); i ++) {
        harmonic_counter += grads[i];
    }
    int harmonic_appear = (harmonic_counter > 4) ? 1 : 0;
    if (harmonic_appear > 0) {
        myFeedbackDetect->pitch_ltm = myFeedbackDetect->pitch_ltm * 0.95 + peak_lb * 0.05;
    }
    if (valid_energy_est < 0.25 * myFeedbackDetect->energy_ltm) {
        harmonic_counter = 10;
        harmonic_appear = 0;
    }

    memcpy(myFeedbackDetect->frame_energy, myFeedbackDetect->frame_energy+1, (FRAME_COUNT_IN_WINDOW - 1)*sizeof(double));
    myFeedbackDetect->frame_energy[(FRAME_COUNT_IN_WINDOW - 1)] = energy_fb + energy_lb;

    memcpy(myFeedbackDetect->lbr_array, myFeedbackDetect->lbr_array+1, (FRAME_COUNT_IN_WINDOW - 1)*sizeof(double));
    myFeedbackDetect->lbr_array[(FRAME_COUNT_IN_WINDOW - 1)] = lbr;

    memcpy(myFeedbackDetect->peak_hbs, myFeedbackDetect->peak_hbs+1, (FRAME_COUNT_IN_WINDOW - 1)*sizeof(double));
    myFeedbackDetect->peak_hbs[(FRAME_COUNT_IN_WINDOW - 1)] = peak_hb;

    memcpy(myFeedbackDetect->peak_lbs, myFeedbackDetect->peak_lbs+1, (FRAME_COUNT_IN_WINDOW - 1)*sizeof(double));
    myFeedbackDetect->peak_lbs[(FRAME_COUNT_IN_WINDOW - 1)] = peak_lb;

    memcpy(myFeedbackDetect->den_times, myFeedbackDetect->den_times+1, (FRAME_COUNT_IN_WINDOW - 1)*sizeof(double));
    myFeedbackDetect->den_times[(FRAME_COUNT_IN_WINDOW - 1)] = (peak_density) / (nonPeak_desity + 1.0f);

    memcpy(myFeedbackDetect->peak_idxs, myFeedbackDetect->peak_idxs+1, (FRAME_COUNT_IN_WINDOW - 1)*sizeof(int));
    myFeedbackDetect->peak_idxs[(FRAME_COUNT_IN_WINDOW - 1)] = peak_index_hb;

    //    energy_ltm = energy_ltm * 0.99 + frame_energy[FRAME_COUNT_IN_WINDOW - 1] * 0.01;
    
    int hangover_howl = 0;
    int hangover_norm = 0;
    int mt_p = 0;
    int can_add = 1;
    double alpha = 1.0;
    double belt = 0.0;
    double  peak_idx_avg = myFeedbackDetect->peak_idxs[0];
    int feedbackAppear = 1;
    
    double avg_frame_energy = 0;
    int counter = 0;
    for (i = 0; i <  FRAME_COUNT_IN_WINDOW; i ++) {
        if (fabs(myFeedbackDetect->peak_idxs[i] - peak_idx_avg) < 11.0f) {
            alpha = 0.2;
            belt = 0.8;
        } else {
            alpha = 0.8;
            belt = 0.2;
        }
        if ((((fabs(myFeedbackDetect->peak_idxs[i] - peak_idx_avg) < 7.5) && (myFeedbackDetect->den_times[i] > 12.0f) && (myFeedbackDetect->lbr_array[i] > 0.8f)) || ((fabs(myFeedbackDetect->peak_idxs[i] - peak_idx_avg) < 5.5) && (myFeedbackDetect->den_times[i] > 7.0f) && (myFeedbackDetect->lbr_array[i] > 0.5f))) && (myFeedbackDetect->peak_hbs[i] > myFeedbackDetect->peak_hb_ltm * 0.05) && (myFeedbackDetect->peak_hbs[i] > 2*myFeedbackDetect->peak_lbs[i])) {
            hangover_howl ++;
            avg_frame_energy += (myFeedbackDetect->frame_energy[i] - myFeedbackDetect->energy_ltm) * (myFeedbackDetect->frame_energy[i] - myFeedbackDetect->energy_ltm);
            counter ++;
            if (hangover_howl > 3) {
                hangover_norm = 0;
            }
            if ((hangover_howl > 9) && (can_add > 0)) {
                mt_p ++;
                can_add = 0;
            }
            if ((hangover_howl > 14) || (mt_p > 1)) {
                feedbackAppear = 2;
            }
        } else if (myFeedbackDetect->peak_lbs[i] > myFeedbackDetect->peak_lb_ltm * 0.1) {
            hangover_norm ++;
            if (hangover_norm > 3) {
                hangover_howl = 0;
                can_add = 1;
            }
            if (hangover_norm > 49) {
                mt_p = 0;
                feedbackAppear = 0;
            }
        }
        peak_idx_avg = myFeedbackDetect->peak_idxs[i] * alpha + peak_idx_avg * belt;
    }
    
    avg_frame_energy = (counter > 0) ? sqrt(avg_frame_energy/counter)/myFeedbackDetect->energy_ltm : 0.0;
    double var_frame_energy = 0;
    counter = 0;
    for (i = 0; i < FRAME_COUNT_IN_WINDOW; i ++) {
        if (myFeedbackDetect->frame_energy[i] > 1) {
            counter ++;
            var_frame_energy += (myFeedbackDetect->frame_energy[i] - myFeedbackDetect->energy_ltm) * (myFeedbackDetect->frame_energy[i] - myFeedbackDetect->energy_ltm);
        }
    }
    double var_frame_energy_norm = sqrt(var_frame_energy/FRAME_COUNT_IN_WINDOW)/myFeedbackDetect->energy_ltm;
    
    //        NSLog(@"sec:%fs; \n {en_lb = %f; en_hb = %f; lbr = %f; pe_de = %f; nonP_de = %f; pe_idx = %d}", second, energy_lb, energy_fb, lbr, peak_density, nonPeak_desity, peak_index_hb);
    
    if (myFeedbackDetect->start_counter > FRAME_COUNT_IN_WINDOW - 2) {
        //            NSLog(@"sec:%fs; var_frame_energy_norm = %f, avg_frame_energy = %f", second, var_frame_energy_norm, avg_frame_energy);
        if (avg_frame_energy < 0.5 || var_frame_energy < 0.5 || harmonic_counter > 4) {
            feedbackAppear = 1;
        }
        myFeedbackDetect->start_counter = FRAME_COUNT_IN_WINDOW - 2;
        if ((energy_fb < 2 * energy_lb) && ((energy_fb + energy_lb) > 0.85 *  myFeedbackDetect->energy_ltm) && (myFeedbackDetect->peak_hb_ltm < 1.25 * peak_lb) && (harmonic_counter > 4) && (fdStatus || (feedbackAppear == 2))) {
            feedbackAppear = 0;
        }
    } else {
        feedbackAppear = 1;
    }
    
    myFeedbackDetect->peak_lb_ltm = myFeedbackDetect->peak_lb_ltm * 0.98 + peak_lb * 0.02;
    
    if (((feedbackAppear == 2) || fdStatus) && (myFeedbackDetect->frame_energy[FRAME_COUNT_IN_WINDOW-1] - peak_density*3) > 0.1*myFeedbackDetect->energy_ltm) {
        myFeedbackDetect->energy_ltm = myFeedbackDetect->energy_ltm * 0.99 + (myFeedbackDetect->frame_energy[FRAME_COUNT_IN_WINDOW-1] - peak_density*3) * 0.01;
    } else if((!fdStatus || (feedbackAppear == 0)) && (myFeedbackDetect->frame_energy[FRAME_COUNT_IN_WINDOW-1] > 0.1*myFeedbackDetect->energy_ltm)) {
        myFeedbackDetect->energy_ltm = myFeedbackDetect->energy_ltm * 0.99 + myFeedbackDetect->frame_energy[FRAME_COUNT_IN_WINDOW-1] * 0.01;
    }
    
    //    alpha = 0.999;
    //    belt = 0.001;
    if (feedbackAppear == 2) {
        
        alpha = 0.85;
        belt = 0.15;
        
    } else if (feedbackAppear == 0) {
        alpha = 1;
        belt = 0;
        memset(myFeedbackDetect->lbr_array, 0, (FRAME_COUNT_IN_WINDOW>>1)*sizeof(double));
        memset(myFeedbackDetect->peak_lbs, 0, (FRAME_COUNT_IN_WINDOW>>1)*sizeof(double));
        memset(myFeedbackDetect->peak_hbs, 0, (FRAME_COUNT_IN_WINDOW>>1)*sizeof(double));
        memset(myFeedbackDetect->den_times, 0, (FRAME_COUNT_IN_WINDOW>>1)*sizeof(double));
        memset(myFeedbackDetect->peak_idxs, 0, (FRAME_COUNT_IN_WINDOW>>1)*sizeof(int));
    } else {
        alpha = 0.95;
        belt = 0.05;
    }
    myFeedbackDetect->peak_hb_ltm = myFeedbackDetect->peak_hb_ltm * alpha + peak_hb * belt;
    
    if (myFeedbackDetect->frameOffset > 0) {
        memcpy(myFeedbackDetect->signal_input, (myFeedbackDetect->signal_input + myFeedbackDetect->frame_len), myFeedbackDetect->frame_len*sizeof(short));
    }
    myFeedbackDetect->frameOffset = myFeedbackDetect->frame_len;
    myFeedbackDetect->start_counter ++;
    myFeedbackDetect->feedbackAppear = feedbackAppear;
    free(grads);
    grads = NULL;
    return feedbackAppear;
}

void closeFeedbackDetect(struct feedbackDetect *myFeedbackDetect)
{
    if (myFeedbackDetect->lbr_array) {
        free(myFeedbackDetect->lbr_array);
        myFeedbackDetect->lbr_array = NULL;
    }
    
    if (myFeedbackDetect->peak_lbs) {
        free(myFeedbackDetect->peak_lbs);
        myFeedbackDetect->peak_lbs = NULL;
    }
    
    if (myFeedbackDetect->peak_hbs) {
        free(myFeedbackDetect->peak_hbs);
        myFeedbackDetect->peak_hbs = NULL;
    }
    
    if (myFeedbackDetect->den_times) {
        free(myFeedbackDetect->den_times);
        myFeedbackDetect->den_times = NULL;
    }
    
    if (myFeedbackDetect->peak_idxs) {
        free(myFeedbackDetect->peak_idxs);
        myFeedbackDetect->peak_idxs = NULL;
    }
    
    if (myFeedbackDetect->signal_input) {
        free(myFeedbackDetect->signal_input);
        myFeedbackDetect->signal_input = NULL;
    }
    
    if (myFeedbackDetect->frame_energy) {
        free(myFeedbackDetect->frame_energy);
        myFeedbackDetect->frame_energy = NULL;
    }

    if (myFeedbackDetect->myOouraFFT) {
        freeOouraFFT(myFeedbackDetect->myOouraFFT);
        free(myFeedbackDetect->myOouraFFT);
        myFeedbackDetect->myOouraFFT = NULL;
    }

    if (myFeedbackDetect) {
        free(myFeedbackDetect);
        myFeedbackDetect = NULL;
    }

}
