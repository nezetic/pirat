/**
 * Ported from Valdemar Erlingsson original C# implementation.
 *
 * https://github.com/ValdemarOrn/SharpSoundPlugins/tree/master/Rodent.V2
 */
#pragma once
#ifndef PRAT_TRANSFER_H
#define PRAT_TRANSFER_H

#ifdef __cplusplus

#include <cstddef>
#include <array>

namespace prat
{

/**
 * A basic class representing a transfer function.
 * To alter the TF method update() should be overloaded to calculate
 * the correct response. If variable comonents are used then setParam() can be used
 * to set the value, and their values then used in update() to calculate the
 * correct response.
 *
 * b[0]*z^3 + b[1]*z^2 + b[2]*z + b[3]   b[0] + b[1]*z^-1 + b[2]*z^-2 + b[3]*z^-3
 * ----------------------------------- = -----------------------------------------
 * a[0]*z^3 + a[1]*z^2 + a[2]*z + a[3]   a[0] + a[1]*z^-1 + a[2]*z^-2 + a[3]*z^-3
 *
 */
template<std::size_t T>
class Transfer
{
    public:

        Transfer() {}

        inline void Init() {
            a.fill(0.f);
            b.fill(0.f);
            bufInL.fill(0.f);
            bufInR.fill(0.f);
            bufOutL.fill(0.f);
            bufOutR.fill(0.f);
        }

        inline int GetOrder() const {
            return b.size() > a.size() ? b.size() - 1 : a.size() - 1;
        }

        inline const std::array<float, T>& GetB() const {
            return b;
        }

        inline void SetB(const std::array<float, T>& val) {
            b = val;
        }

        inline const std::array<float, T>& GetA() const {
            return a;
        }

        inline void SetA(const std::array<float, T>& val) {
            a = val;
            if (a[0] == 0.0)
                gain = 0.0;
            else
                gain = 1 / a[0];
        }

        virtual void Update() {
        }

        void Process(
            const float* inputL,
            const float* inputR,
            float* outputL,
            float* outputR,
            size_t len)
        {
            for (size_t i = 0; i < len; i++) {
                index = (index + 1) % modulo;

                bufInL[index] = inputL[i];
                if (inputR != nullptr) {
                    bufInR[index] = inputR[i];
                } else {
                    bufInR[index] = 0;
                }
                bufOutL[index] = 0;
                bufOutR[index] = 0;

                for (typename std::array<float, T>::size_type j = 0; j < b.size(); ++j) {
                    bufOutL[index] += (b[j] * bufInL[((index - j) + modulo) % modulo]);
                    bufOutR[index] += (b[j] * bufInR[((index - j) + modulo) % modulo]);
                }

                for (typename std::array<float, T>::size_type j = 1; j < a.size(); ++j) {
                    bufOutL[index] -= (a[j] * bufOutL[((index - j) + modulo) % modulo]);
                    bufOutR[index] -= (a[j] * bufOutR[((index - j) + modulo) % modulo]);
                }

                bufOutL[index] = bufOutL[index] * gain;
                bufOutR[index] = bufOutR[index] * gain;

                outputL[i] = bufOutL[index];
                if (outputR != nullptr) {
                    outputR[i] = bufOutR[index];
                }
            }
        }

        inline float Process(float input) {
            float output = 0.f;
            Process(&input, nullptr, &output, nullptr, 1);
            return output;
        }

    private:
        int index = 0;

        float gain = 0.0;

        static constexpr int modulo = 50;

        std::array<float, T> b;
        std::array<float, T> a;

        std::array<float, modulo> bufInL;
        std::array<float, modulo> bufInR;
        std::array<float, modulo> bufOutL;
        std::array<float, modulo> bufOutR;
};

} // namespace prat
#endif
#endif
