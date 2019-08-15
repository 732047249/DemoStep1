// import from zayhu-VoIP/jni/webrtc-3.46/webrtc/modules/audio_processing/high_pass_filter_impl.cc
class HighPassFilter
{
public:
    HighPassFilter(int sample_rate_hz);
    ~HighPassFilter();

    int filter(int16_t* data, int length);

private:
    int16_t mStateY[4];
    int16_t mStateX[2];
    const int16_t* mStateBA;
};
