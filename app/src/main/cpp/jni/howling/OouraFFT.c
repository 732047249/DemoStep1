//
//  OouraFFT.c
//  YeeCall-VoIP
//
//  Created by David on 16/8/5.
//  Copyright © 2016年 YeeCall. All rights reserved.
//

#include <math.h>

#include "OouraFFT.h"

void initForSignalsOfLength(struct OouraFFT *myOouraFFT, int numPoints, int windowType)
{
    myOouraFFT->dataLength = 0;
    if (!checkDataLength(numPoints)) {
//        printf("FFT size must be the power of 2, and less than 32767");
        return;
    }
    myOouraFFT->dataLength = numPoints;
    myOouraFFT->numFrequencies = numPoints/2;
    
    // Initialize helper arrays the FFT algorithm requires.
    myOouraFFT->ip = (int *)malloc((2 + sqrt(myOouraFFT->numFrequencies)) * sizeof(int));
    myOouraFFT->ip[0] = 0; // Need to set this before the first time we run the FFT
    
    
    myOouraFFT->w  = (double *)malloc((myOouraFFT->numFrequencies) * sizeof(double));
    
    // Initialize the raw input and output data arrays
    // In use, when calculating periodograms, outputData will not generally
    // be used. Look to realFrequencyData for that.
    myOouraFFT->inputData = (double *)malloc(myOouraFFT->dataLength*sizeof(double));
    
    // Initialize the data we'd actually display
    myOouraFFT->spectrumData = (double *)malloc(myOouraFFT->numFrequencies*sizeof(double));
    
    // Allocate the windowing function
    myOouraFFT->window = (double *)malloc(myOouraFFT->dataLength*sizeof(double));
    
    // And then set the window type with our custom setter.
    myOouraFFT->windowType = windowType;
    
    setWindowType(myOouraFFT, windowType);

}

int checkDataLength(int inDataLength)
{
    // Check that inDataLength is a power of two.
    // Thanks StackOverflow! (Q 600293, answered by Greg Hewgill)
    int isPowerOfTwo = (inDataLength & (inDataLength - 1)) == 0;
    // and that it's not too long. INT OVERFLOW BAD.
    int isWithinIntRange = inDataLength < MAX_INT;
    
    return isPowerOfTwo & isWithinIntRange;
}

void setWindowType(struct OouraFFT *myOouraFFT, int inWindowType)
{
    myOouraFFT->windowType = inWindowType;
    double N = myOouraFFT->dataLength;
    double windowval, tmp;
    double pi = 3.14159265359;
    
    // Source: http://en.wikipedia.org/wiki/Window_function
    int i;
    for (i=0; i < myOouraFFT->dataLength; ++i) {
        switch (myOouraFFT->windowType) {
            case kWindow_Rectangular:
                windowval = 1.0;
                break;
            case kWindow_Hamming:
                windowval = 0.54 - 0.46*cos((2*pi*i)/(N - 1));
                break;
            case kWindow_Hann:
                windowval = 0.5*(1 - cos((2*pi*i)/(N - 1)));
                break;
            case kWindow_Bartlett:
                windowval  = (N-1)/2;
                tmp = i - ((N-1)/2);
                if (tmp < 0.0) {tmp = -tmp;}
                windowval -= tmp;
                windowval *= (2/(N-1));
                break;
            case kWindow_Triangular:
                tmp = i - ((N-1.0)/2.0);
                if (tmp < 0.0) {tmp = -tmp;} 
                windowval = (2/N)*((N/2) - tmp);
                break;
            default:
                windowval = 1.0;
                break;
        }
        myOouraFFT->window[i] = windowval;
    }
}

void windowSignalSegment(struct OouraFFT *myOouraFFT)
{
    int i;
    for (i=0; i<myOouraFFT->dataLength; ++i) {
        myOouraFFT->inputData[i] *= myOouraFFT->window[i];
    }
}

void calculateOourFFT(struct OouraFFT *myOouraFFT, int isFFT)
{
    int i;
    // Apply a window to the signal segment
    windowSignalSegment(myOouraFFT);
    
    // Do the FFT
    rdft(myOouraFFT->dataLength, isFFT, myOouraFFT->inputData, myOouraFFT->ip, myOouraFFT->w);

    // Get the real modulus of the FFT, and scale it
    for (i=0; i<myOouraFFT->numFrequencies; ++i) {
        myOouraFFT->spectrumData[i] = sqrt(myOouraFFT->inputData[i*2]*myOouraFFT->inputData[i*2] + myOouraFFT->inputData[i*2+1]*myOouraFFT->inputData[i*2+1]);
    }
}

void freeOouraFFT(struct OouraFFT *myOouraFFT)
{
    free(myOouraFFT->inputData);
    free(myOouraFFT->spectrumData);
    free(myOouraFFT->ip);
    free(myOouraFFT->w);
    free(myOouraFFT->window);

    myOouraFFT->inputData = NULL;
    myOouraFFT->spectrumData = NULL;
    myOouraFFT->ip = NULL;
    myOouraFFT->w = NULL;
    myOouraFFT->window = NULL;
}
