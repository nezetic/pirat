/**
 * Ported from Valdemar Erlingsson original C# implementation.
 *
 * https://github.com/ValdemarOrn/SharpSoundPlugins/tree/master/Rodent.V2
 */
#pragma once
#ifndef PIRAT_SPLINEINTERPOLATOR_H
#define PIRAT_SPLINEINTERPOLATOR_H

#ifdef __cplusplus

#include <algorithm>
#include <array>

namespace pirat
{

template<std::size_t T>
class SplineInterpolator
{
    public:

        SplineInterpolator(const std::array<std::array<float, T>, 3>& spline):
            xs(spline[0]), ys(spline[1]), ks(spline[2]) {
            max = std::max_element(xs.begin(), xs.end())[0];
            min = std::min_element(xs.begin(), xs.end())[0];
        }

        void Process(
            const float* inputL,
            const float* inputR,
            float* outputL,
            float* outputR,
            size_t len)
        {
            const bool stereo = inputR != nullptr && outputR != nullptr;
            for (size_t i = 0; i < len; i++) {
                outputL[i] = Interpolate(inputL[i]);
                if (stereo) {
                    outputR[i] = Interpolate(inputR[i]);
                }
            }
        }

        inline float Process(float input) {
            float output = 0.f;
            Process(&input, nullptr, &output, nullptr, 1);
            return output;
        }

        float Bias = 0.f;

    private:

        inline float Interpolate(float input) {
            float x = input + Bias;
            if (x > max) x = max - 0.00001;
            if (x < min) x = min + 0.00001;
            float y = InterpolateNoBias(x);
            return y;
        }

        inline float InterpolateNoBias(float x) {
            float i = 1;
            while (xs[i] < x)
                i++;

            float t = (x - xs[i - 1]) / (xs[i] - xs[i - 1]);

            float a = ks[i - 1] * (xs[i] - xs[i - 1]) - (ys[i] - ys[i - 1]);
            float b = -ks[i] * (xs[i] - xs[i - 1]) + (ys[i] - ys[i - 1]);

            float q = (1 - t) * ys[i - 1] + t * ys[i] + t * (1 - t) * (a * (1 - t) + b * t);
            return q;
        }

        float min, max;
        const std::array<float, T> &xs, &ys, &ks;
};

} // namespace pirat
#endif
#endif
