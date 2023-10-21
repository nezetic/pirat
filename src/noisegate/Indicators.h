/**
 * Based on the amazing NoiseInvaderVST from ValdemarOrn.
 *
 * https://github.com/ValdemarOrn/NoiseInvaderVST
 */
#pragma once

#ifndef PIRAT_INDICATORS_H
#define PIRAT_INDICATORS_H

#include "Utils.h"

#ifdef __cplusplus

#include <array>

namespace pirat
{
    class Sma
    {
    private:

        // 1024 is enough for 10ms at 96k sample rate
        std::array<double, 1024> queue;
        int sampleCount;

        int head;
        double sum;
        double dbDecayPerSample;

    public:

        Sma() {}

        void Init(size_t sampleCount);

        double GetDbDecayPerSample() const {
            return dbDecayPerSample;
        }

        double Update(double sample);
    };

    class Ema
    {
    private:
        double alpha;
        double value;

    public:

        Ema() {}

        void Init(double alpha) {
            this->alpha = alpha;
        }

        double Update(double sample) {
            value = sample * alpha + value * (1 - alpha);
            return value;
        }
    };

    class EmaLatch
    {
    private:
        double alpha;
        double latch;
        double value;
        double currentValue;

    public:

        EmaLatch() {}

        void Init(double alpha, double latch) {
            this->alpha = alpha;
            this->latch = latch;
        }

        double Update(bool input) {
            double sample = input ? 1.0 : -1.0;
            value = sample * alpha + value * (1 - alpha);

            if (value > latch)
                currentValue = 1.0;
            if (value < -latch)
                currentValue = -1.0;

            return currentValue;
        }
    };
}

#endif
#endif // PIRAT_INDICATORS_H
