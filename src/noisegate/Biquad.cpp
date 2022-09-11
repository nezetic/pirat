/**
 * Based on the amazing NoiseInvaderVST from ValdemarOrn.
 *
 * https://github.com/ValdemarOrn/NoiseInvaderVST
 */
#include <cmath>

#include "Biquad.h"
#include "Utils.h"

namespace prat
{
    void Biquad::Init(FilterType filterType, int samplerate) {
        Type = filterType;
        SetSamplerate(samplerate);

        SetGainDb(0.0);
        SetFreq((float)(samplerate / 4.0));
        SetQ(0.5);
        ClearBuffers();
    }

    int Biquad::GetSamplerate() const {
        return _samplerate;
    }

    void Biquad::SetSamplerate(int value) {
        _samplerate = value; 
        Update();
    }

    float Biquad::GetGainDb() const {
        return std::log10(gain) * 20;
    }

    void Biquad::SetGainDb(float value) {
        SetGain(Utils::DB2gain(value));
    }

    float Biquad::GetGain() const {
        return gain;
    }

    void Biquad::SetGain(float value) {
        if (value < 0.001f)
            value = 0.001f; // -60dB
        
        gain = value;
    }

    float Biquad::GetFreq() const {
        return _freq;
    }

    void Biquad::SetFreq(float value) {
        _freq = value;
    }

    float Biquad::GetQ() const {
        return _q;
    }

    void Biquad::SetQ(float value) {
        if (value < 0.001f)
            value = 0.001f;
        _q = value;
    }

    float Biquad::GetSlope() const {
        return _slope;
    }

    void Biquad::SetSlope(float value) {
        _slope = value;
    }

    void Biquad::Update() {
        float omega = (float)(2 * M_PI * _freq / _samplerate);
        float sinOmega = Utils::FastSin(omega);
        float cosOmega = Utils::FastCos(omega);

        float sqrtGain = 0.0;
        float alpha = 0.0;

        if (Type == FilterType::LowShelf || Type == FilterType::HighShelf) {
            alpha = sinOmega / 2 * std::sqrt((gain + 1 / gain) * (1 / _slope - 1) + 2);
            sqrtGain = std::sqrt(gain);
        } else {
            alpha = sinOmega / (2 * _q);
        }

        switch (Type)
        {
        case FilterType::LowPass:
            b0 = (1 - cosOmega) / 2;
            b1 = 1 - cosOmega;
            b2 = (1 - cosOmega) / 2;
            a0 = 1 + alpha;
            a1 = -2 * cosOmega;
            a2 = 1 - alpha;
            break;
        case FilterType::HighPass:
            b0 = (1 + cosOmega) / 2;
            b1 = -(1 + cosOmega);
            b2 = (1 + cosOmega) / 2;
            a0 = 1 + alpha;
            a1 = -2 * cosOmega;
            a2 = 1 - alpha;
            break;
        case FilterType::BandPass:
            b0 = alpha;
            b1 = 0;
            b2 = -alpha;
            a0 = 1 + alpha;
            a1 = -2 * cosOmega;
            a2 = 1 - alpha;
            break;
        case FilterType::Notch:
            b0 = 1;
            b1 = -2 * cosOmega;
            b2 = 1;
            a0 = 1 + alpha;
            a1 = -2 * cosOmega;
            a2 = 1 - alpha;
            break;
        case FilterType::Peak:
            b0 = 1 + (alpha * gain);
            b1 = -2 * cosOmega;
            b2 = 1 - (alpha * gain);
            a0 = 1 + (alpha / gain);
            a1 = -2 * cosOmega;
            a2 = 1 - (alpha / gain);
            break;
        case FilterType::LowShelf:
            b0 = gain * ((gain + 1) - (gain - 1) * cosOmega + 2 * sqrtGain * alpha);
            b1 = 2 * gain * ((gain - 1) - (gain + 1) * cosOmega);
            b2 = gain * ((gain + 1) - (gain - 1) * cosOmega - 2 * sqrtGain * alpha);
            a0 = (gain + 1) + (gain - 1) * cosOmega + 2 * sqrtGain * alpha;
            a1 = -2 * ((gain - 1) + (gain + 1) * cosOmega);
            a2 = (gain + 1) + (gain - 1) * cosOmega - 2 * sqrtGain * alpha;
            break;
        case FilterType::HighShelf:
            b0 = gain * ((gain + 1) + (gain - 1) * cosOmega + 2 * sqrtGain * alpha);
            b1 = -2 * gain * ((gain - 1) + (gain + 1) * cosOmega);
            b2 = gain * ((gain + 1) + (gain - 1) * cosOmega - 2 * sqrtGain * alpha);
            a0 = (gain + 1) - (gain - 1) * cosOmega + 2 * sqrtGain * alpha;
            a1 = 2 * ((gain - 1) - (gain + 1) * cosOmega);
            a2 = (gain + 1) - (gain - 1) * cosOmega - 2 * sqrtGain * alpha;
            break;
        }

        float g = 1 / a0;

        b0 = b0 * g;
        b1 = b1 * g;
        b2 = b2 * g;
        a1 = a1 * g;
        a2 = a2 * g;
    }

    float Biquad::GetResponse(float freq) const {
        double phi = std::pow((std::sin(2 * M_PI * freq / (2.0 * _samplerate))), 2);
        return (float)((std::pow(b0 + b1 + b2, 2.0) - 4.0 * (b0 * b1 + 4.0 * b0 * b2 + b1 * b2) * phi + 16.0 * b0 * b2 * phi * phi) / (std::pow(1.0 + a1 + a2, 2.0) - 4.0 * (a1 + 4.0 * a2 + a1 * a2) * phi + 16.0 * a2 * phi * phi));
    }

    void Biquad::ClearBuffers() {
        y = 0;
        x2 = 0;
        y2 = 0;
        x1 = 0;
        y1 = 0;
    }
}
