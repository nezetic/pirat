/**
   * Based on the amazing NoiseInvaderVST from ValdemarOrn.
   *
   * https://github.com/ValdemarOrn/NoiseInvaderVST
   */
#pragma once

#ifndef PRAT_SLEWLIMITER_H
#define PRAT_SLEWLIMITER_H

#ifdef __cplusplus

namespace prat
{
    class SlewLimiter
    {
    private:
        double fs;
        double slewUp;
        double slewDown;

        double output = 0.f;

    public:

        SlewLimiter() {}

        void Init(double fs) {
            this->fs = fs;
            this->slewUp = 1;
            this->slewDown = 1;
        }

        /// <summary>
        /// Computes the slew rates for fading 60 dB in the time specified
        /// </summary>
        void UpdateDb60(double slewUpMillis, double slewDownMillis) {
            double upSamples = slewUpMillis / 1000.0 * fs;
            double downSamples = slewDownMillis / 1000.0 * fs;
            this->slewUp = 60.0 / upSamples;
            this->slewDown = 60.0 / downSamples;
        }

        double Process(double value) {
            if (value > output) {
                if (value > output + slewUp)
                    output = output + slewUp;
                else
                    output = value;
            } else {
                if (value < output - slewDown)
                    output = output - slewDown;
                else
                    output = value;
            }

            return output;
        }
    };
}

#endif
#endif // PRAT_SLEWLIMITER_H
