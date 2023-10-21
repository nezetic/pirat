#pragma once
#ifndef PIRAT_GAIN_H
#define PIRAT_GAIN_H

#include "Transfer.h"
#include "Utils.h"

#ifdef __cplusplus

namespace pirat
{

class TFGain : public Transfer<4>
{
    public:

        TFGain() {};

        /** Initializes the TFGain module.
         * \param sample_rate - the sample rate of the audio engine being run.
         */
        void Init(float sample_rate);

        /** Update sample rate.
         * \param sample_rate - the sample rate of the audio engine being run.
         */
        inline void SetSampleRate(float sample_rate) {
            sample_rate_ = sample_rate;
        }

        /** Sets the gain parameter.
         *  \param gain - gain parameter value. Range: Any positive value.
         */
        inline void SetGain(float gain)
        {
            float prev = gain_;
            gain_ = gain;
            if (Utils::NotNearlyEqual(prev, gain)) {
                Update();
            }
        }

        /** Sets the ruetz mod parameter.
         *  \param state - enable / disable RUETZ mode.
         */
        inline void SetRuetz(bool state)
        {
            bool prev = ruetz_;
            ruetz_ = state;
            if (prev != state) {
                Update();
            }
        }

        void Update() override;

        /**
          \return the current value for the gain.
          */
        inline float GetGain() const { return gain_; }

    private:
        float gain_ = 0.0f;
        float sample_rate_ = 44100.f;
        bool ruetz_ = false;
};

} // namespace pirat
#endif
#endif
