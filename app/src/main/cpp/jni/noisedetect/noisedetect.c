//
// Created by 杨锐 on 2017/12/5.
//

#include "noisedetect.h"
#include "logcat.h"
/*
void openfile()
{
    fEnv = fopen("/sdcard/yeecall/env.dat","wb+");
    fAbs = fopen("/sdcard/yeecall/abs.pcm","wb+");
    fTmp = fopen("/sdcard/yeecall/tmp.dat","wb+");
}
void closefile()
{
     fclose(fEnv);
   fclose(fAbs);
   fclose(fTmp);
}
*/
void filter(const float* x, float* y, int xlen, float* a, float* b, int nfilt)
{
    float tmp;
    int i,j;

    //normalization
    if( (*a-1.0>EPS) || (*a-1.0<-EPS) )
    {
        tmp=*a;
        for(i=0;i<nfilt;i++)
        {
            a[i]/=tmp;
        }
         b[0]/=tmp;
    }

    memset(y,0,xlen*sizeof(float));

    a[0]=0.0;
    for(i=0;i<xlen;i++)
    {
        for(j=0;i>=j&&j<nfilt;j++)
        {
            if(j>0)
            {
                y[i] += -a[j]*y[i-j];
            }
            else
            {
                y[i] += (b[j]*x[i-j]-a[j]*y[i-j]);
            }
        }
    }
    a[0]=1.0;

}
//return 1: strong noise detected
//return -1: no strong noise
//return 0: preparing envolop for noise detection
int doNoiseDetect(short* input, int size, int nFrameCount)
{
    int nEnvolopBegin;
    float fAver = 0.0f;
    float fSilence = 0.0f;
    int nBigCount = 0;
    int nSmallCount = 0;
    int i=0;
    int nFrame = nFrameCount % FRAME_NUM;

    nEnvolopBegin = nFrame * FRAME_LENGTH / 8;

    //resample envolope as one of every 8 samples
    for(i=4; i < size; i+=8){
          mAbs[i/8 + nEnvolopBegin] = 2 * abs(input[i]);
          mEnvolope[i/8 + nEnvolopBegin] = mAbs[i/8 + nEnvolopBegin];
    }

    //do noise detection every  FRAME_NUM  frames
    if(nFrame == FRAME_NUM-1){

        //generate envolope using butter filter
        filter(mAbs, mEnvolope, ENVOLOPE_LEN, coefA, coefB, 5);

        // if(NULL != fTmp)
       //  {
        //     fwrite(mAbs, sizeof(float), ENVOLOPE_LEN, fTmp);
        //     LOGI("%s_%d   Writting tmp data…………",__FILE__,__LINE__);
        //  }

      //  if(NULL != fEnv)
      //  {
       //     LOGI("%s_%d   Writting env data…………",__FILE__,__LINE__);
       //     fwrite(mEnvolope, sizeof(float), ENVOLOPE_LEN, fEnv);
       //  }

        if(nFrameCount == FRAME_NUM-1){
            for(i=0; i < 100*FRAME_LENGTH / 8; i++){
                           fSilence += mEnvolope[i];
                       }
            fSilence /= (100*FRAME_LENGTH);

            //Set Silence threshold
            mSilenceMax = (SILENCE_VALUE > fSilence * 1.5) ? SILENCE_VALUE : fSilence * 1.5;
        }

         for(i=0; i < ENVOLOPE_LEN; i++){
               fAver += mEnvolope[i];
           }

        fAver /= ENVOLOPE_LEN;

        for(i=0; i < ENVOLOPE_LEN; i++){
            if(mEnvolope[i] > fAver * BIG_THRETHOLD && fAver > mSilenceMax){
                nBigCount ++;
               }
            if(mEnvolope[i] < fAver * BIG_THRETHOLD/2 && fAver > mSilenceMax){
                nSmallCount ++;
               }
        }
        //strong noise detected
        if (nBigCount > NOISE_RATIO_MIN_DEFAULTS * ENVOLOPE_LEN && nSmallCount < ENVOLOPE_LEN * 0.3){
             return  1;
         }
         //no strong noise
         else
             return -1;
    }
    //return 0 means preparing envolope for noise detecting
      return 0;
}
/*
//Noise reduce ratio after Suppression
float getNoiseReduce(short* before, short* after, int size)
{
    float fAverBefore = 0.0f;
    float fAverAfter = 0.0f;
    int nNoiseBefore = 0;
    int nNoiseAfter = 0;
    float fNoiseReduce = 0.0f;
    int i = 0;

    for(i = 0; i < size; i++){
        fAverBefore += abs(before[i]);
        fAverAfter += abs(after[i]);
    }
    fAverBefore /= size;
    fAverAfter /= size;

    for(i = 0; i < size; i++){
        if(fAverBefore > mSilenceMax * 2 && abs(before[i]) > fAverBefore * 0.2  && abs(before[i]) < fAverBefore * 0.7 )
            nNoiseBefore ++;

        if(fAverAfter > mSilenceMax * 2 && abs(after[i]) > fAverAfter * 0.2  && abs(after[i]) < fAverAfter * 0.7 )
            nNoiseAfter ++;
    }

    fNoiseReduce = nNoiseBefore - nNoiseAfter;

    fNoiseReduce /= size;

    return fNoiseReduce;
}
*/
//Noise ratio of input
float getNoiseRatio(short* input,  int size)
{
    float fAver = 0.0f;
    int nNoise = 0;
    float fNoiseRatio = 0.0f;
    int i = 0;

    for(i = 0; i < size; i++){
        fAver += abs(input[i]);
    }
    fAver /= size;

    for(i = 0; i < size; i++){
        if(fAver > mSilenceMax * 2 && abs(input[i]) > fAver * NOISE_LOW  && abs(input[i]) < fAver * NOISE_HIGH )
            nNoise ++;
    }

    fNoiseRatio = nNoise;

    fNoiseRatio /= size;

    return fNoiseRatio;
}
