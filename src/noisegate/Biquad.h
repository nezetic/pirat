/**
 * Based on the amazing NoiseInvaderVST from ValdemarOrn.
 *
 * https://github.com/ValdemarOrn/NoiseInvaderVST
 */
#pragma once

#ifndef PRAT_BIQUAD
#define PRAT_BIQUAD

#ifdef __cplusplus

namespace prat
{
    class Biquad
    {
    public:

        enum class FilterType {
            LowPass = 0,
            HighPass,
            BandPass,
            Notch,
            Peak,
            LowShelf,
            HighShelf
        };

    private:

        int _samplerate = 0.f;
        float _freq = 0.f;
        float _q = 0.f;
        float _slope = 0.f;

        float a0, a1, a2, b0, b1, b2;
        float x1, x2, y, y1, y2;

        float gain = 0.f;

    public:

        FilterType Type = FilterType::LowPass;

        Biquad() {};

        void Init(FilterType filterType, int samplerate);

        int GetSamplerate() const;
        void SetSamplerate(int samplerate);
        float GetGainDb() const;
        void SetGainDb(float value);
        float GetGain() const;
        void SetGain(float value);
        float GetFreq() const;
        void SetFreq(float value);
        float GetQ() const;
        void SetQ(float value);
        float GetSlope() const;
        void SetSlope(float value);

        void Update();
        float GetResponse(float freq) const;
        
        inline float Process(float x) {
            y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1;
            y2 = y1;
            x1 = x;
            y1 = y;

            return y;
        }

        inline void Process(float* input, float* output, int len) {
            for (int i = 0; i < len; i++) {
                float x = input[i];
                y = ((b0 * x) + (b1 * x1) + (b2 * x2)) - (a1 * y1) - (a2 * y2);
                x2 = x1;
                y2 = y1;
                x1 = x;
                y1 = y;

                output[i] = y;
            }
        }

        void ClearBuffers();
    };
}

#endif
#endif // PRAT_BIQUAD
