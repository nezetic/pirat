#pragma once
#ifndef PIRAT_LOWPASS1_H
#define PIRAT_LOWPASS1_H

#include "Transfer.h"
#include "Utils.h"

#ifdef __cplusplus

namespace pirat
{

class Lowpass1 : public Transfer<2>
{
    public:

        Lowpass1() {};

        /** Initializes the Lowpass1 module.
         * \param sample_rate - the sample rate of the audio engine being run.
         */
        void Init(float sample_rate);

        /** Update sample rate.
         * \param sample_rate - the sample rate of the audio engine being run.
         */
        inline void SetSampleRate(float sample_rate) {
            sample_rate_ = sample_rate;
        }

        /** Sets the cutoff frequency or half-way point of the filter.
         *  \param freq - frequency value in Hz. Range: Any positive value.
         */
        inline void SetFreq(float freq) {
            float prev = freq_;
            freq_ = freq;
            if (Utils::NotNearlyEqual(prev, freq)) {
                Update();
            }
        }

        void Update() override;

        /**
          \return the current value for the cutoff frequency or half-way point of the filter.
          */
        inline float GetFreq() const { return freq_; }

    private:
        float freq_ = 0.0f;
        float sample_rate_ = 44100.f;
};

} // namespace pirat
#endif
#endif
