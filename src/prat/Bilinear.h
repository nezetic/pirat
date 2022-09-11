/**
 * Ported from Valdemar Erlingsson original C# implementation.
 *
 * https://github.com/ValdemarOrn/SharpSoundPlugins/tree/master/Rodent.V2
 */
#pragma once
#ifndef PRAT_BILINEAR_H
#define PRAT_BILINEAR_H

#include "DSP.h"

#ifdef __cplusplus

#include <algorithm>
#include <array>
#include <vector>

namespace prat
{

template<std::size_t T>
struct BilinearCommon {

    static inline void Init(std::array<float, T>& zb, std::array<float, T>& za) {
        zb.fill(0.f);
        za.fill(0.f);
    }

    static inline void Invert(std::array<float, T>& zb, std::array<float, T>& za) {
        float g = za[0];
        float gInv = 1 / g;

        for (typename std::array<float, T>::size_type i = 0; i < zb.size(); i++) {
            za[i] *= gInv;
            zb[i] *= gInv;
        }
    }
};

/**
 * Takes data on the form
 * b3*s^3 + b2*s^2 + b1*s + b0
 * ---------------------------  
 * a3*s^3 + a2*s^2 + a1*s + a0
 * Warning: b[0] = b0, b[1] = b1, etc, so make sure array is not reversed
 * 
 */
template<std::size_t T>
class Bilinear
{
    public:
        Bilinear() = delete;

        static void Transform(std::array<float, T>& b, std::array<float, T>& a, std::array<float, T>& zb, std::array<float, T>& za, float fs) {
            BilinearCommon<T>::Init(zb, za);

            Bilinear::Supertransform(b, a, zb, za, fs);

            BilinearCommon<T>::Invert(zb, za);
        }

    private:

        static void Supertransform(const std::array<float, T>& b, const std::array<float, T>& a, std::array<float, T>& zb, std::array<float, T>& za, float fs) {
            int numOfCoeffs = b.size();
            int order = numOfCoeffs - 1;

            std::vector<float> zplus1 = { 1.0f, 1.0f };
            std::vector<float> zminus1 = { 1.0f, -1.0f };

            std::vector<std::vector<float>> polys(numOfCoeffs);

            for (int i=0; i < numOfCoeffs; i++) {
                polys[i] = Bilinear::Conv(ArrayPower(zplus1, order-i) , ArrayPower(zminus1, i));
            }

            for (int i=0; i < numOfCoeffs; i++) {
                zb[i] = 0;
                za[i] = 0;
                for (int j=0; j < numOfCoeffs; j++) {
                    zb[i] += (float)(polys[j][i]*b[j]*DSP::powf(2*fs, j));
                    za[i] += (float)(polys[j][i]*a[j]*DSP::powf(2*fs, j));
                }

            }
        }

        static std::vector<float> ArrayPower(const std::vector<float>& a, size_t n)
        {
            std::vector<float> output = { 1.f };
            for (size_t i = 0; i < n; i++) {
                output = Bilinear::Conv(output, a);
            }
            return output;
        }

        static std::vector<float> Conv(const std::vector<float>& h, const std::vector<float>& g)
        {
            std::vector<float> output(h.size() + g.size() - 1);

            // To minimize the number of MAC-operations, split the loop in two
            // parts, use different algorithms on each side
            std::vector<float>::size_type lg = g.size();
            std::vector<float>::size_type lh = h.size();
            for (std::vector<float>::size_type i = 0; i < output.size(); i++) {
                for (std::vector<float>::size_type j = 0; j <= i; j++) {
                    if (j < lh && (i - j) < lg)
                        output[i] += h[j] * g[i - j];
                }
            }
            return output;
        }
};


template<>
class Bilinear<2>
{
    public:
        Bilinear() = delete;

        static void Transform(std::array<float, 2>& b, std::array<float, 2>& a, std::array<float, 2>& zb, std::array<float, 2>& za, float fs) {
            BilinearCommon<2>::Init(zb, za);

            Bilinear::StoZ1(b, a, zb, za, fs);

            BilinearCommon<2>::Invert(zb, za);
        }

    private:

        static inline void StoZ1(const std::array<float, 2>& b, const std::array<float, 2>& a, std::array<float, 2>& zb, std::array<float, 2>& za, float fs) {
            zb[1] = b[0] - 2 * b[1] * fs;   //z^0
            zb[0] = b[0] + 2 * b[1] * fs;   //z^1

            za[1] = a[0] - 2 * a[1] * fs;   //z^0
            za[0] = a[0] + 2 * a[1] * fs;   //z^1
        }
};


template<>
class Bilinear<3>
{
    public:
        Bilinear() = delete;

        static void Transform(std::array<float, 3>& b, std::array<float, 3>& a, std::array<float, 3>& zb, std::array<float, 3>& za, float fs) {
            BilinearCommon<3>::Init(zb, za);

            Bilinear::StoZ2(b, a, zb, za, fs);

            BilinearCommon<3>::Invert(zb, za);
        }

    private:

        static inline void StoZ2(const std::array<float, 3>& b, const std::array<float, 3>& a, std::array<float, 3>& zb, std::array<float, 3>& za, float fs) {
            float fs2 = fs * fs;

            zb[2] = b[0] - 2 * b[1] * fs + 4 * b[2] * fs2;  //z^0
            zb[1] = 2 * b[0] - 8 * b[2] * fs2;  //z^1
            zb[0] = b[0] + 2 * b[1] * fs + 4 * b[2] * fs2;  //z^2

            za[2] = a[0] - 2 * a[1] * fs + 4 * a[2] * fs2;  //z^0
            za[1] = 2 * a[0] - 8 * a[2] * fs2;  //z^1
            za[0] = a[0] + 2 * a[1] * fs + 4 * a[2] * fs2;  //z^2
        }
};


template<>
class Bilinear<4>
{
    public:
        Bilinear() = delete;

        static void Transform(std::array<float, 4>& b, std::array<float, 4>& a, std::array<float, 4>& zb, std::array<float, 4>& za, float fs) {
            BilinearCommon<4>::Init(zb, za);

            Bilinear::StoZ3(b, a, zb, za, fs);

            BilinearCommon<4>::Invert(zb, za);
        }

    private:

        static inline void StoZ3(const std::array<float, 4>& b, const std::array<float, 4>& a, std::array<float, 4>& zb, std::array<float, 4>& za, float fs) {
            float fs2 = fs * fs;
            float fs3 = fs * fs * fs;

            zb[3] = b[0] - 2 * b[1] * fs + 4 * b[2] * fs2 - 8 * b[3] * fs3; //z^0
            zb[2] = 3 * b[0] - 2 * b[1] * fs - 4 * b[2] * fs2 + 24 * b[3] * fs3;    //z^1
            zb[1] = 3 * b[0] + 2 * b[1] * fs - 4 * b[2] * fs2 - 24 * b[3] * fs3;    //z^2
            zb[0] = b[0] + 2 * b[1] * fs + 4 * b[2] * fs2 + 8 * b[3] * fs3; //z^3

            za[3] = a[0] - 2 * a[1] * fs + 4 * a[2] * fs2 - 8 * a[3] * fs3; //z^0
            za[2] = 3 * a[0] - 2 * a[1] * fs - 4 * a[2] * fs2 + 24 * a[3] * fs3;    //z^1
            za[1] = 3 * a[0] + 2 * a[1] * fs - 4 * a[2] * fs2 - 24 * a[3] * fs3;    //z^2
            za[0] = a[0] + 2 * a[1] * fs + 4 * a[2] * fs2 + 8 * a[3] * fs3; //z^3
        }
};


template<>
class Bilinear<5>
{
    public:
        Bilinear() = delete;

        static void Transform(std::array<float, 5>& b, std::array<float, 5>& a, std::array<float, 5>& zb, std::array<float, 5>& za, float fs) {
            BilinearCommon<5>::Init(zb, za);

            Bilinear::StoZ4(b, a, zb, za, fs);

            BilinearCommon<5>::Invert(zb, za);
        }

    private:

        static inline void StoZ4(const std::array<float, 5>& b, const std::array<float, 5>& a, std::array<float, 5>& zb, std::array<float, 5>& za, float fs) {
            float fs2 = fs * fs;
            float fs3 = fs * fs * fs;
            float fs4 = fs * fs * fs * fs;

            zb[4] = b[0] - 2 * b[1] * fs + 4 * b[2] * fs2 - 8 * b[3] * fs3 + 16 * b[4] * fs4;   //z0
            zb[3] = 4 * b[0] - 4 * b[1] * fs + 16 * b[3] * fs3 - 64 * b[4] * fs4;   //z1
            zb[2] = 6 * b[0] - 8 * b[2] * fs2 + 96 * b[4] * fs4;    //z2
            zb[1] = 4 * b[0] + 4 * b[1] * fs - 16 * b[3] * fs3 - 64 * b[4] * fs4;   //z3
            zb[0] = b[0] + 2 * b[1] * fs + 4 * b[2] * fs2 + 8 * b[3] * fs3 + 16 * b[4] * fs4;   //z4

            za[4] = a[0] - 2 * a[1] * fs + 4 * a[2] * fs2 - 8 * a[3] * fs3 + 16 * a[4] * fs4;   //z0
            za[3] = 4 * a[0] - 4 * a[1] * fs + 16 * a[3] * fs3 - 64 * a[4] * fs4;   //z1
            za[2] = 6 * a[0] - 8 * a[2] * fs2 + 96 * a[4] * fs4;    //z2
            za[1] = 4 * a[0] + 4 * a[1] * fs - 16 * a[3] * fs3 - 64 * a[4] * fs4;   //z3
            za[0] = a[0] + 2 * a[1] * fs + 4 * a[2] * fs2 + 8 * a[3] * fs3 + 16 * a[4] * fs4;   //z4
        }
};


} // namespace prat
#endif
#endif
