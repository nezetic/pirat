/**
 * Versio implementation of PRat distortion, featuring:
 *
 * - stereo signal path;
 * - knobs with dedicated CV controls;
 * - hard clip, ruetz and tight mods;
 * - envelope modulation of gain / filter (with attenuverters);
 * - noise gate (with a bypass and adjustable threshold / release).
 *
 */
#include "daisy_versio.h"
#include "daisysp.h"

#include "PRatDist.h"
#include "NoiseGate.h"
#include "GrabValue.h"

using namespace daisy;
using namespace daisysp;

using namespace prat;


struct Settings
{
    Settings():
        ng_threshold(0.4f),  // -45db
        ng_release(0.5f) {}    // 100ms

    float ng_threshold;
    float ng_release;

    bool operator==(const Settings &rhs) {
        return Utils::NearlyEqual(ng_threshold, rhs.ng_threshold)
            && Utils::NearlyEqual(ng_release, rhs.ng_release);
    }
    bool operator!=(const Settings &rhs) { return !operator==(rhs); }
};

// Hardware object for the versio
DaisyVersio hw;

// Settings
PersistentStorage<Settings> storage(hw.seed.qspi);
Settings default_settings;
bool dosave = false;
bool initialized = false;

// PRat distortion
PRatDist dist;
// PRat noise gate
NoiseGate ng;

// UX values
float envVal = 0.f;
float satVal = 0.f;


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    static bool first = true;
    static bool prevshift = false;

    static GrabValue<float> cv_mix = 0.f;
    static GrabValue<float> cv_level = 0.f;

    static GrabValue<float> ng_threshold = 0.4f;  // -45db
    static GrabValue<float> ng_release = 0.5f;    // 100ms

    hw.ProcessAllControls();
    hw.tap.Debounce();

    // pass-thru until module is initialized
    if (!initialized) {
        Utils::Copy(IN_L, IN_R, OUT_L, OUT_R, size);
        return;
    }

    const bool shift = hw.SwitchPressed() && !first;

    const float cv0 = hw.GetKnobValue(DaisyVersio::KNOB_0);
    const float cv1 = hw.GetKnobValue(DaisyVersio::KNOB_1);

    if (first) {
        Settings &settings = storage.GetSettings();

        ng_threshold.Update(settings.ng_threshold);
        ng_release.Update(settings.ng_release);

    } else if (!shift && prevshift) { // save settings when exiting shift mode
        Settings &settings = storage.GetSettings();

        settings.ng_threshold = ng_threshold.Get();
        settings.ng_release = ng_release.Get();

        dosave = true;
    }

    if (!shift) {
        cv_mix.Update(cv0);
        cv_level.Update(cv1);

        ng_threshold.Lock();
        ng_release.Lock();
    } else {
        cv_mix.Lock();
        cv_level.Lock();

        ng_threshold.Update(cv0);
        ng_release.Update(cv1);
    }

    // noise gate envelope follower signal (boosted to be in 0-1v range)
    const float env = fclamp(ng.GetEnvelope() * 2.5f, 0.f, 1.f);
    // bipolar knob to modulate filter based on envelope follower
    const float filter_mod = Utils::Detented(fclamp(hw.GetKnobValue(DaisyVersio::KNOB_3), 0.f, 1.f), 0.5f) - 0.5f;
    const float filter_mod_cv = env * filter_mod * 2.f;
    // bipolar knob to modulate gain based on envelope follower
    const float gain_mod = Utils::Detented(fclamp(hw.GetKnobValue(DaisyVersio::KNOB_5), 0.f, 1.f), 0.5f) - 0.5f;
    const float gain_mod_cv = env * gain_mod * 2.f;

    const float gain = fclamp(hw.GetKnobValue(DaisyVersio::KNOB_4) + gain_mod_cv, 0.f, 1.f);
    const float filter = fclamp(hw.GetKnobValue(DaisyVersio::KNOB_2) + filter_mod_cv, 0.f, 1.f);
    const float level = fclamp(cv_level.Get(), 0.f, 1.f);
    const float mix = fclamp(cv_mix.Get(), 0.f, 1.f);
    const float volume = fclamp(hw.GetKnobValue(DaisyVersio::KNOB_6), 0.f, 1.f);

    const int sw_clip = hw.sw[DaisyVersio::SW_0].Read();
    const int sw_mod = hw.sw[DaisyVersio::SW_1].Read();

    const float hard = (sw_clip == Switch3::POS_CENTER || hw.Gate()) ? 1.f : 0.f;
    const float bypass = sw_clip == Switch3::POS_DOWN ? 1.f : 0.f;
    const float ruetz = sw_mod == Switch3::POS_CENTER ? 1.f : 0.f;
    const float tight = sw_mod == Switch3::POS_DOWN ? 1.f : 0.f;

    dist.SetParam(PRatDist::P_GAIN, gain);
    dist.SetParam(PRatDist::P_FILTER, filter);
    dist.SetParam(PRatDist::P_LEVEL, level);
    // in hard mode, mix Silicon / Led clippers
    if (hard >= 0.5f) {
        dist.SetParam(PRatDist::P_DRYWET, 1.f);
        dist.SetParam(PRatDist::P_SILED, mix);
    } else {
        dist.SetParam(PRatDist::P_DRYWET, mix);
    }
    dist.SetParam(PRatDist::P_GAIN_IN, volume);
    dist.SetParam(PRatDist::P_HARD, hard);
    dist.SetParam(PRatDist::P_TIGHT, tight);
    dist.SetParam(PRatDist::P_RUETZ, ruetz);
    dist.SetParam(PRatDist::P_BYPASS, bypass);

    dist.Update();

    if (first || shift) {
        ng.SetParam(NoiseGate::P_THRESHOLD, ng_threshold.Get());
        ng.SetParam(NoiseGate::P_RELEASE, ng_release.Get());
        // a threshold < -75db disable the noise gate
        ng.SetParam(NoiseGate::P_BYPASS, ng_threshold.Get() < 0.05f ? 1.f : 0.f);

        ng.Update();
    }

    dist.Process(IN_L, IN_R, OUT_L, OUT_R, size);
    // noise gate uses left input for volume detection
    ng.Process(OUT_L, OUT_R, IN_L, OUT_L, OUT_R, size);

    // output the distortion saturation
    satVal = fclamp(dist.GetSaturation() / 5.f, 0.f, 1.f);
    envVal = env;

    first = false;
    prevshift = shift;
}


int main(void)
{
    static bool first = true;

    hw.Init();
    //hw.StartLog();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(4);

    storage.Init(default_settings);

    const float sr = hw.AudioSampleRate();
    dist.Init(sr);
    ng.Init(sr);

    ng.SetParam(NoiseGate::P_DETECTOR_GAIN, 0.5);  // * 1
    ng.SetParam(NoiseGate::P_REDUCTION, 0.4);      // -40db
    ng.SetParam(NoiseGate::P_SLOPE, 0.3, true);    // 3

    uint32_t boottime = System::GetNow();

    hw.StartAudio(AudioCallback);
    hw.StartAdc();

    while (1) {
        if (!initialized) {
            if (System::GetNow() - boottime < 1000) {
                // starting with button pressed restore default settings
                if (hw.SwitchPressed()) {
                    Settings &settings = storage.GetSettings();
                    settings = default_settings;
                    dosave = true;
                }
                if (first) {
                    hw.SetLed(DaisyVersio::LED_0, 1.f, 0.f, 0.f);
                    hw.SetLed(DaisyVersio::LED_1, 1.f, 0.f, 1.f);
                    hw.SetLed(DaisyVersio::LED_2, 1.f, 0.f, 0.f);
                    hw.SetLed(DaisyVersio::LED_3, 1.f, 0.f, dosave ? 0.f : 1.f);
                    hw.UpdateLeds();

                    first = false;
                }
            } else {
                initialized = true;
            }
        } else {
            hw.SetLed(DaisyVersio::LED_0, 0.f, envVal, 0.f);
            hw.SetLed(DaisyVersio::LED_1, 0.f, envVal, 0.f);
            hw.SetLed(DaisyVersio::LED_2, satVal, 0.f, 0.f);
            hw.SetLed(DaisyVersio::LED_3, satVal, 0.f, 0.f);
            hw.UpdateLeds();
            // save settings
            if (dosave) {
                storage.Save();
                dosave = false;
            }
        }
    }
}
