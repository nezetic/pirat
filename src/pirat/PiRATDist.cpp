#include "PiRATDist.h"
#include "Utils.h"

using namespace pirat;


void PiRATDist::Init(float sample_rate) {
    Hipass1.Init(sample_rate);
    Lowpass1.Init(sample_rate);
    Gain.Init(sample_rate);
    HipassDC.Init(sample_rate);
    Filter.Init(sample_rate);
    Hipass3.Init(sample_rate);

    SetSampleRate(sample_rate);

    // Frequency of 0.01uF cap + 1k + 1Meg = 7.227 Hz
    Hipass1.SetFreq(10.f + p_tight_ * 300.f);
    // Low pass rolloff because of 1n cap, estimate
    Lowpass1.SetFreq(5000.f);
    // This is the cap after the gain, just some value to remove DC offset
    HipassDC.SetFreq(10.f);
    // Final cutoff frequency ~ 7.7Hz
    Hipass3.SetFreq(7.7f);

#if HAS_FADER_SUPPORT
    MixerDryWet.Init();
    MixerSiLed.Init();
#endif

    Update();
}


void PiRATDist::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;

    Hipass1.SetSampleRate(sample_rate);
    Lowpass1.SetSampleRate(sample_rate);
    Gain.SetSampleRate(sample_rate);
    HipassDC.SetSampleRate(sample_rate);
    Filter.SetSampleRate(sample_rate);
    Hipass3.SetSampleRate(sample_rate);

    Update();
}


void PiRATDist::Update() {
    Hipass1.SetFreq(10.f + p_tight_ * 300.f);
    Gain.SetGain(Utils::ExpResponse(p_gain_));
    Gain.SetRuetz(p_ruetz_ >= 0.5f);

    float freq = (1.0 / (2.0 * M_PI * 0.0033e-6 * (2500 + 100000 * Utils::ExpResponse(p_filter_)))); // Range: 19292.0Hz to 470.0Hz
    Filter.SetFreq(freq);
#if HAS_FADER_SUPPORT
    MixerDryWet.SetPos(p_drywet_);
    MixerSiLed.SetPos(p_siled_);
#endif
}


void PiRATDist::Process(
        const float* inputL,
        const float* inputR,
        float* outputL,
        float* outputR,
        size_t len) {

    const bool stereo = inputR != nullptr && outputR != nullptr;
    if (len < 1) {
        return;
    }

    const float gainIn = Utils::ExpResponse(p_gain_in_);
    Utils::Gain(inputL, inputR, gainIn, outputL, outputR, len);

    Hipass1.Process(outputL, outputR, outputL, outputR, len);

    const float gainM3 = Utils::DB2gain(-3);
    Utils::Gain(outputL, outputR, gainM3, outputL, outputR, len);

    Lowpass1.Process(outputL, outputR, outputL, outputR, len);

    Gain.Process(outputL, outputR, outputL, outputR, len);

    // LM308 has a voltage swing of about +-4 volt, then it hard clips
    float ref = outputL[0];
    Utils::Saturate(outputL, outputR, 4.0f, outputL, outputR, len);
    sat_ = abs(ref - outputL[0]);

    HipassDC.Process(outputL, outputR, outputL, outputR, len);

    Utils::Saturate(outputL, outputR, 7.99f, outputL, outputR, len);

    for (size_t i = 0; i < len; i++) {
        float tmpL, tmpR = 0.f;
        float* signalL = &outputL[i];
        float* signalR = stereo ? &outputR[i] : nullptr;

        Clipper.Process(signalL, signalR, &tmpL, &tmpR, 1); // Silicon

        if (p_hard_ >= 0.5f) {
            Clipper2.Process(signalL, signalR, signalL, signalR, 1); // LED
            Utils::Gain(signalL, signalR, 0.7f, signalL, signalR, 1);
#if HAS_FADER_SUPPORT
            MixerSiLed.Process(signalL, signalR, &tmpL, &tmpR, signalL, signalR, 1);
#endif
        } else {
            *signalL = tmpL;
            if (stereo) {
                *signalR = tmpR;
            }
        }
    }

    // OD
    //signal = 0.6f * signal + 0.9f * clean;

    Filter.Process(outputL, outputR, outputL, outputR, len);
    Hipass3.Process(outputL, outputR, outputL, outputR, len);

    const float gainOut = Utils::ExpResponse(p_level_);
    Utils::Gain(outputL, outputR, gainOut, outputL, outputR, len);

#if HAS_FADER_SUPPORT
    MixerDryWet.Process(inputL, inputR, outputL, outputR, outputL, outputR, len);
#endif

    if (p_bypass_ > 0.5f) {
        Utils::Copy(inputL, inputR, outputL, outputR, len);
    }
}

