/**
 * Based on the amazing NoiseInvaderVST from ValdemarOrn.
 *
 * https://github.com/ValdemarOrn/NoiseInvaderVST
 */
#pragma once

#ifndef PIRAT_ENVFOLLOWER_H
#define PIRAT_ENVFOLLOWER_H

#include "Biquad.h"
#include "Indicators.h"
#include "OnePoleFilters.h"
#include "Utils.h"

#ifdef __cplusplus

namespace pirat
{
    class EnvelopeFollower
    {
    private:
        static constexpr double InputFilterHpCutoff = 100.0;
        static constexpr double InputFilterCutoff = 2000.0;
        static constexpr double EmaFc = 200.0;
        static constexpr double SmaPeriodSeconds = 0.01; // 10ms
        static constexpr double TimeoutPeriodSeconds = 0.01; // 10ms
        static constexpr double HoldSmootherFc = 200.0;

        double Fs = 0.f;
        double ReleaseMs = 0.f;

        Hp1 hpFilter;
        Biquad inputFilter;
        Sma sma;
        Ema ema;
        EmaLatch movementLatch;

        int triggerCounterTimeoutSamples = 0;

        double slowDecay = 0.f;
        double fastDecay = 0.f;
        double holdAlpha = 0.f;

        double hold = 0.f;
        int lastTriggerCounter = 0;
        double h1 = 0.f;
        double h2 = 0.f;
        double h3 = 0.f;
        double h4 = 0.f;

    public:

        EnvelopeFollower() {}

        void Init(double fs, double releaseMs);

        void SetRelease(double releaseMs) {
            ReleaseMs = releaseMs;
            double dbDecayPerSample = -60 / (ReleaseMs / 1000.0 * Fs);
            fastDecay = Utils::DB2gain(dbDecayPerSample);
        }

        double ProcessEnvelope(double val);

    };
}

#endif
#endif // PIRAT_ENVFOLLOWER_H
