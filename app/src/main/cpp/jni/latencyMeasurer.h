#ifndef Header_latencyMeasurer
#define Header_latencyMeasurer

typedef enum MeasurementStates {
    enumMeasureAverageLoudness = 0,
    enumPlayingAndListening = 1,
    enumWaiting = 2,
    enumPassthrough = 3,
    enumIdle = 4
} MeasurementStates;

class latencyMeasurer {
public:
    MeasurementStates mCurrentMeasurementState, mNextMeasurementState;
    int mMeasureCount;
    int mSampleRate;
    int mLatencyMs;

    // constructor
    latencyMeasurer();

    // input & output
    void processInput(short int *audio, int samplerate, int numberOfSamples);
    void processOutput(short int *audio, int samplerate, int numberOfSamples);

    void startMeasuring(int count);

private:
    int mBufferSize;
    int mThreshold;

    float mRoundTripLatencyMs[32];

    float mSineWave;
    float mRampDec;

    int mSum;
    int mSamplesElapsed;

    int mRetryCount;
    int mLouderSpeaking;
    float mAmplitude;
};

#endif
