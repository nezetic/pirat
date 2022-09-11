#pragma once
#ifndef PRAT_UTILS_H
#define PRAT_UTILS_H

#include "DSP.h"

#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#endif

#ifdef __cplusplus

#include <cassert>
#include <cfloat>

namespace prat
{

class Utils
{
  public:
    Utils() = delete;

    static inline float ExpResponse(float input) {
        return (DSP::powf(20.f, input)-1)/19;
    }

    static inline float DecResponse(float input) {
        return DSP::powf(100.f, input)/100.f;
    }

    static inline float OctResponse(float input) {
        return (DSP::powf(4, input) - 1.0) / 4.0 + 0.25;
    }

    static inline float Detented(float val, float mid, float lim = 0.01f) {
        return (val > (mid + lim) || val < (mid - lim)) ? val : mid;
    }

    static inline void Copy(
            const float* inputL,
            const float* inputR,
            float* outputL,
            float* outputR,
            size_t len) {

        const bool stereo = inputR != nullptr && outputR != nullptr;
        for (size_t i = 0; i < len; i++) {
            outputL[i] = inputL[i];
            if (stereo) {
                outputR[i] = inputR[i];
            }
        }
    }

    static inline void Gain(
            const float* inputL,
            const float* inputR,
            const float gain,
            float* outputL,
            float* outputR,
            size_t len) {

        const bool stereo = inputR != nullptr && outputR != nullptr;
        for (size_t i = 0; i < len; i++) {
            outputL[i] = Utils::Gain(inputL[i], gain);
            if (stereo) {
                outputR[i] = Utils::Gain(inputR[i], gain);
            }
        }
    }

    static inline float Gain(float input, float gain) {
        return input * gain;
    }

    static inline float DB2gain(float input) {
        return DSP::pow10f(input/20);
    }

    static inline float Gain2DB(float input) {
        return 20.0f * std::log10(input);
    }

    static inline void Saturate(
            const float* inputL,
            const float* inputR,
            const float min,
            const float max,
            float* outputL,
            float* outputR,
            size_t len) {

        const bool stereo = inputR != nullptr && outputR != nullptr;
        for (size_t i = 0; i < len; i++) {
            outputL[i] = Utils::Saturate(inputL[i], min, max);
            if (stereo) {
                outputR[i] = Utils::Saturate(inputR[i], min, max);
            }
        }
    }

    static inline void Saturate(
            const float* inputL,
            const float* inputR,
            const float max,
            float* outputL,
            float* outputR,
            size_t len) {
        return Utils::Saturate(inputL, inputR, -max, max, outputL, outputR, len);
    }

    static inline float Saturate(float input, float max) {
        return Saturate(input, -max, max);
    }

    static inline float Saturate(float input, float min, float max) {
        return DSP::fclamp(input, min, max);
    }

    static inline float FastSin(float x)
    {
        // TODO
        return sin(x);
    }

    static inline float FastCos(float x)
    {
        // TODO
        return cos(x);
    }

    static inline double ComputeLpAlpha(double fc, double ts)
    {
        return (2 * M_PI * ts * fc) / (2 * M_PI * ts * fc + 1);
    }

    static inline bool NearlyEqual(float a, float b, float epsilon = 128 * FLT_EPSILON, float abs_th = FLT_MIN) {
        assert(std::numeric_limits<float>::epsilon() <= epsilon);
        assert(epsilon < 1.f);

        if (a == b) return true;

        float diff = std::abs(a - b);
        float norm = DSP::fmin(std::abs(a + b), std::numeric_limits<float>::max());
        return diff < DSP::fmax(abs_th, epsilon * norm);
    }

    static inline bool NotNearlyEqual(float a, float b, float epsilon = 128 * FLT_EPSILON, float abs_th = FLT_MIN) {
        return !NearlyEqual(a, b, epsilon, abs_th);
    }

    static inline void PreventDernormals()
    {
#if defined(__i386__) || defined(__x86_64__)
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
    }
};

} // namespace prat
#endif
#endif
