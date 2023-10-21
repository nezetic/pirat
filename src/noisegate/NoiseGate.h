/**
   * Based on the amazing NoiseInvaderVST from ValdemarOrn.
   *
   * https://github.com/ValdemarOrn/NoiseInvaderVST
   */
#pragma once

#ifndef PIRAT_NOISEGATE_H
#define PIRAT_NOISEGATE_H

#include "Expander.h"
#include "EnvelopeFollower.h"
#include "SlewLimiter.h"
#include "Utils.h"

#ifdef __cplusplus

namespace pirat
{
/** Noise gate module

Based on the amazing Valdemar Erlingsson NoiseInvader VST.

*/
    class NoiseGate
    {
    public:
        static constexpr int P_DETECTOR_GAIN = 0;
        static constexpr int P_REDUCTION = 1;
        static constexpr int P_RELEASE = 2;
        static constexpr int P_SLOPE = 3;
        static constexpr int P_THRESHOLD = 4;
        static constexpr int P_BYPASS = 5;

    private:

        float fs = 0.f;

        EnvelopeFollower envelopeFollower;
        Expander expander;
        SlewLimiter slewLimiter;

        float p_bypass_ = 0.f;

        // Gain Settings
        float p_detector_gain = 0.f;

        // Noise Gate Settings
        double p_reduction_db = 0.f;
        double p_threshold_db = 0.f;
        double p_slope_ = 0.f;
        double p_release_ms = 0.f;

        // for readouts
        double current_gain_ = 0.f;
        double current_env_ = 0.f;

    public:

        NoiseGate() {}

        void Init(int fs) {
            p_detector_gain = 1.0f; // 0.5
            p_reduction_db = -100;  // 1.0
            p_threshold_db = -40;   // 0.5
            p_slope_ = 3;           // 0.3
            p_release_ms = 100;     // 0.5

            SetSampleRate(fs);
        }

        void Update() {
            expander.Update(p_threshold_db, p_reduction_db, p_slope_);
            envelopeFollower.SetRelease(p_release_ms);
            slewLimiter.UpdateDb60(2.0, p_release_ms);
        }

        void Process(
            const float* inputL,
            const float* inputR,
            const float* detectorInput,
            float* outputL,
            float* outputR,
            size_t len);

        inline float Process(float input, float detector) {
            float output = 0.f;
            Process(&input, nullptr, &detector, &output, nullptr, 1);
            return output;
        }

        inline float Process(float input) {
            return Process(input, input);
        }

        void SetSampleRate(float sample_rate) {
            this->fs = sample_rate;

            envelopeFollower.Init(sample_rate, p_release_ms);
            slewLimiter.Init(sample_rate);

            Update();
        }

        inline void SetParam(int param, float value, bool update = false) {
            value = DSP::fclamp(value, 0.f, 1.f);
            switch (param) {
                case P_DETECTOR_GAIN:
                    p_detector_gain = Utils::DB2gain(40 * value - 20);
                    break;
                case P_REDUCTION:
                    p_reduction_db = -value * 100;
                    break;
                case P_RELEASE:
                    p_release_ms = 10 + Utils::DecResponse(value) * 990;
                    break;
                case P_SLOPE:
                    p_slope_ = 1.0f + Utils::DecResponse(value) * 50;
                    break;
                case P_THRESHOLD:
                    p_threshold_db = -Utils::OctResponse(1 - value) * 80;
                    break;
                case P_BYPASS:
                    p_bypass_ = value;
                    break;
                default:
                    update = false;
            }

            if (update) {
                Update();
            }
        }

        inline float GetGain() const {
            return current_gain_;
        }

        inline float GetEnvelope() const {
            return current_env_;
        }
    };
}

#endif
#endif // PIRAT_NOISEGATE_H
