#pragma once
#ifndef PRAT_DIST_H
#define PRAT_DIST_H

#include "DSP.h"

#include "CrossFade.h"
#include "Highpass1.h"
#include "Lowpass1.h"
#include "Splines.h"
#include "SplineInterpolator.h"
#include "TFGain.h"

#ifdef USE_DAISYSP
#define HAS_FADER_SUPPORT 1
#else
#define HAS_FADER_SUPPORT 0
#endif

#ifdef __cplusplus

namespace prat
{
/** ProCo Rat 2 distortion module

Ported from the amazing Valdemar Erlingsson Rodent V2 VST.

*/
class PRatDist
{
  public:
    PRatDist(): Clipper(Splines::D1N914TF), Clipper2(Splines::LEDTF) {}
    ~PRatDist() {}

    static constexpr int P_GAIN    = 0;
    static constexpr int P_FILTER  = 1;
    static constexpr int P_LEVEL   = 2;
    static constexpr int P_DRYWET  = 3;
    static constexpr int P_GAIN_IN = 4;
    static constexpr int P_BYPASS  = 5;
    static constexpr int P_HARD    = 6;
    static constexpr int P_SILED   = 7;
    static constexpr int P_TIGHT   = 8;
    static constexpr int P_RUETZ   = 9;

    /** Initializes the PRat module.
     *  \param sample_rate - the sample rate of the audio engine being run.
     */
    void Init(float sample_rate);

    /** Update distortion based on parameters.
     */
    void Update();

    /** Processes the distortion (stereo).
     */
    void Process(
        const float* inputL,
        const float* inputR,
        float* outputL,
        float* outputR,
        size_t len);

    /** Processes the distortion.
     *  \param in - input signal value.
     */
    inline float Process(float input) {
        float output = 0.f;
        Process(&input, nullptr, &output, nullptr, 1);
        return output;
    }

    /** Update the sample rate.
     *  \param sample_rate - the sample rate of the audio engine being run.
     */
    void SetSampleRate(float sample_rate);

    /** Sets the filter parameter.
     *  \param param - parameter ID.
     *  \param value - parameter Value.
     *  \param update - if true, update distortion based on new parameter value.
     */
    inline void SetParam(int param, float value, bool update = false) {
        value = DSP::fclamp(value, 0.f, 1.f);
        switch(param) {
            case P_GAIN: p_gain_ = value; break;
            case P_FILTER: p_filter_ = value; break;
            case P_GAIN_IN: p_gain_in_ = value; break;
            case P_DRYWET: p_drywet_ = value; break;
            case P_LEVEL: p_level_ = value; break;
            case P_BYPASS: p_bypass_ = value; break;
            case P_HARD: p_hard_ = value; break;
            case P_SILED: p_siled_ = value; break;
            case P_TIGHT: p_tight_ = std::round(value); break;
            case P_RUETZ: p_ruetz_ = std::round(value); break;
        }
        if (update) {
            Update();
        }
    }

    inline float GetSaturation() const {
        return sat_;
    }

  private:
    float p_gain_ = 0.f;
    float p_filter_ = 0.f;
    float p_gain_in_ = 1.f;
    float p_drywet_ = 1.f;
    float p_siled_ = 1.f;
    float p_level_ = 1.f;
    float p_bypass_ = 0.f;
    float p_hard_ = 0.f;
    float p_tight_ = 0.f;
    float p_ruetz_ = 0.f;
    float sat_ = 0;
    float sample_rate_ = 44100.f;

    prat::Highpass1 Hipass1;
    prat::Lowpass1 Lowpass1;
    prat::TFGain Gain;
    prat::Highpass1 HipassDC;
    prat::Lowpass1 Filter;
    prat::Highpass1 Hipass3;

    SplineInterpolator<Splines::D1N914TF_len> Clipper;
    SplineInterpolator<Splines::LEDTF_len> Clipper2;
#if HAS_FADER_SUPPORT
    CrossFade MixerDryWet;
    CrossFade MixerSiLed;
#endif
};
} // namespace prat
#endif
#endif
