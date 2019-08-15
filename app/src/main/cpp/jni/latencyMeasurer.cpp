#include "latencyMeasurer.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "logcat.h"
extern char* dumpAudioSamples(short* memPtr, int sampleCount);

/*
 Cross-platform class measuring round-trip audio latency.
 It runs on both iOS and Android, so the same code performs the measurement on every mobile device.
 How one measurement step works:

 - Listen and measure the average loudness of the environment for 1 second.
 - Create a threshold value 24 decibels higher than the average loudness.
 - Begin playing a 1000 Hz sine wave and start counting the samples elapsed.
 - Stop counting and playing if the input's loudness is higher than the threshold, as the output wave is coming back (probably).
 - Divide the the elapsed samples with the sample rate to get the round-trip audio latency value in seconds.
 - We expect the threshold exceeded within 1 second. If it did not, then fail with error. Usually happens when the environment is too noisy (loud).

 How the measurement process works:

 - Perform MEASURE_ROUNDS (10) measurement steps.
 - Repeat every step until it returns without an error.
 - Store the results in an array of 10 floats.
 - After each step, check the minimum and maximum values.
 - If the maximum is higher than the minimum's double, stop the measurement process with an error. It indicates an unknown error, perhaps an unwanted noise happened. Double jitter (dispersion) is too high, an audio system can not be so bad.
*/

// Returns with the absolute sum of the audio.
static int sumAudio(short int * audio, int numberOfSamples) {
    int sum = 0;
    for (int i = 0; i < numberOfSamples; i++) {
        short int sample = audio[i];
        if (sample > 0) {
            sum += sample;
        } else if (sample < 0) {
            sum -= sample;
        }
    }
    return sum;
}

latencyMeasurer::latencyMeasurer()
    : mCurrentMeasurementState(enumIdle)
    , mNextMeasurementState(enumIdle)
    , mSamplesElapsed(0)
    , mSineWave(0)
    , mSum(0)
    , mThreshold(0)
    , mMeasureCount(0)
    , mSampleRate(0)
    , mLatencyMs(0)
    , mBufferSize(0)
    , mRetryCount(0)
{
    mAmplitude = 32767.0f * 0.3;
}

void latencyMeasurer::startMeasuring(int count) {
    // start in waiting
    mMeasureCount = 0;
    mRetryCount = count;
    mSampleRate = mLatencyMs = mBufferSize = 0;
    mRampDec = -1;
    mNextMeasurementState = enumWaiting;
    LOGI("now set to start measuring");
}

// process recorded frames from mic
void latencyMeasurer::processInput(short int* audio, int samplerate, int numberOfSamples) {
    mSampleRate = samplerate;
    mBufferSize = numberOfSamples;

    if (mNextMeasurementState != mCurrentMeasurementState) {
        if (mNextMeasurementState == enumMeasureAverageLoudness) {
            mSamplesElapsed = 0;
        }

        mCurrentMeasurementState = mNextMeasurementState;
        LOGI("update measurement state to: %d", mCurrentMeasurementState);
    };

    switch (mCurrentMeasurementState) {
    // Measuring average loudness for 1 second.
    case enumMeasureAverageLoudness: {
        mRampDec = -1.0f; // do not generating any sound in waiting
        mSum += sumAudio(audio, numberOfSamples);
        mSamplesElapsed += numberOfSamples;

        // LOGI("read sample average - waiting: %f", float(mSum) / mSamplesElapsed);

        if (mSamplesElapsed >= mSampleRate) { // 1 second elapsed, set up the next step.
            // Look for an audio energy rise of 24 decibel.
            float averageAudioValue = mSum / mAmplitude / mSamplesElapsed;
            float referenceDecibel = 20.0f * log10f(averageAudioValue) + 24.0f;
            mThreshold = (int)(powf(10.0f, referenceDecibel / 20.0f) * mAmplitude);

            LOGI("measure average loudness done with average: %d, +24dB threshold: %d", (int)(averageAudioValue * 32767.0f), mThreshold);

            mCurrentMeasurementState = mNextMeasurementState = enumPlayingAndListening;
            mSineWave = 0;
            mSamplesElapsed = 0;
            mSum = 0;
        };
        break;
    }
    // Playing sine wave and listening if it comes back.
    case enumPlayingAndListening: {
        mRampDec = 0.0f; // start playing

        int averageInputValue = sumAudio(audio, numberOfSamples) / numberOfSamples;
        LOGI("read sample average - play and listen: %d, expect threshold: %d", averageInputValue, mThreshold);

        if (averageInputValue > mThreshold) { // The signal is above the threshold, so our sine wave comes back on the input.
            // Check the location when it became loud enough.
            int n = 0;
            short int *input = audio;
            while (n < numberOfSamples) {
                if (*input++ > mThreshold) {
                    break;
                }
                n++;
            };

            mSamplesElapsed += n; // Now we know the total round trip latency.

            if (mSamplesElapsed > numberOfSamples) {
                // Expect at least 1 buffer of round-trip latency.
                mRoundTripLatencyMs[mMeasureCount] = float(mSamplesElapsed * 1000) / float(mSampleRate);
                mMeasureCount++;

                float sum = 0, max = 0, min = 100000.0f;
                for (int n = 0; n < mMeasureCount; n++) {
                    if (mRoundTripLatencyMs[n] > max) {
                        max = mRoundTripLatencyMs[n];
                    }
                    if (mRoundTripLatencyMs[n] < min) {
                        min = mRoundTripLatencyMs[n];
                    }
                    sum += mRoundTripLatencyMs[n];
                };

                if (max / min > 2.0f) { // Dispersion error.
                    LOGI("failed with dispersion error: %d", mMeasureCount);
                    mLatencyMs = 0;
                    mMeasureCount = -1;
                    mCurrentMeasurementState = mNextMeasurementState = enumPassthrough;
                } else if (mRetryCount == 0 && mMeasureCount > 0) { // Final result.
                    mLatencyMs = sum / mMeasureCount;
                    mCurrentMeasurementState = mNextMeasurementState = enumPassthrough;
                    LOGI("measure latency final result: %d", mLatencyMs);
                } else { // Next step.
                    mLatencyMs = mRoundTripLatencyMs[mMeasureCount - 1];
                    mCurrentMeasurementState = mNextMeasurementState = enumWaiting;
                    LOGI("wait & measure next round: %d, current latency: %d", mMeasureCount, mLatencyMs);
                };
            } else {
                mCurrentMeasurementState = mNextMeasurementState = enumWaiting; // Happens when an early noise comes in.
            }

            mRampDec = 1.0f / float(numberOfSamples);
        } else { // Still listening.
            mSamplesElapsed += numberOfSamples;

            // Do not listen to more than a second, let's start over. Maybe the environment's noise is too high.
            if (mSamplesElapsed > mSampleRate) {
                mRampDec = -1; // stop generating sound
                mCurrentMeasurementState = mNextMeasurementState = enumWaiting;
                mLatencyMs = -1;
                LOGI("could not find echo. might be background noise too high. start over again");
            };
        };
        break;
    }
    case enumPassthrough:
    case enumIdle: {
        mRampDec = -1.0f; // do not generating any sound in waiting

        break;
    }
    // wait in silence for 1 second
    case enumWaiting:
    default: {
        mRampDec = -1.0f; // do not generating any sound in waiting

        // Waiting 1/2 second.
        mSamplesElapsed += numberOfSamples;

        if (mSamplesElapsed > mSampleRate / 2) { // 1 second elapsed, start over.
            mSamplesElapsed = 0;

            if (mRetryCount > 0) {
                mRetryCount--;
                mCurrentMeasurementState = mNextMeasurementState = enumMeasureAverageLoudness;
                // LOGI("wait 0.5 second done. now start over again");
            } else {
                // LOGI("could not found any echo for this device. HW AEC is working properly. now stop measurer");
                mCurrentMeasurementState = mNextMeasurementState = enumPassthrough;
            }
        };
    }
    };
}

// generate pure sine wave for playback
void latencyMeasurer::processOutput(short int *audio, int samplerate, int numberOfSamples) {
    if (mCurrentMeasurementState == enumPassthrough || mCurrentMeasurementState == enumIdle) {
        return;
    }

    mSampleRate = samplerate;
    mBufferSize = numberOfSamples;

    if (mRampDec < 0.0f) {
        // Output silence.
        memset(audio, 0, mBufferSize * sizeof(short int));
    } else {
        // Output sine wave.
        float frequency = 1000.0f;
        float mul = (2.0f * float(M_PI) * frequency) / float(mSampleRate);
        float ramp = 1.0f;
        int n = mBufferSize;

        bool flip = false;

        short int * ptr = audio;
        short int * end = audio + numberOfSamples;
        while (ptr < end) {
            ramp -= mRampDec;
            *ptr = (short int)(sinf(mul * mSineWave + 0.25f * float(M_PI)) * ramp * mAmplitude);
            mSineWave += 1.0f;
            ptr++;
        };

        // LOGI("generate samples: %d, rampDec: %f - %s", sumAudio(audio, numberOfSamples) / numberOfSamples, mRampDec, dumpAudioSamples(audio, numberOfSamples));
    };
}
