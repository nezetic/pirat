/**
 * Patch.init implementation of PRat distortion, featuring:
 *
 * - stereo signal path;
 * - knobs with dedicated CV controls;
 * - hard clip and ruetz mods;
 * - noise gate (with a bypass and adjustable threshold / release);
 * - envelope follower (both raw signal and a gate with adjustable threshold);
 * - trigger to gate (with variable length).
 *
 */
#include "daisy_patch_sm.h"
#include "daisysp.h"

#include "PRatDist.h"
#include "NoiseGate.h"
#include "GrabValue.h"
#include "Gate.h"

using namespace daisy;
using namespace daisysp;
using namespace patch_sm;

using namespace prat;


struct Settings
{
    Settings():
        pr_ruetz(0.f),       // off
        ng_threshold(0.4f),  // -45db
        ng_release(0.5f),    // 100ms
        t2g_length(0.04f),   // ~12ms
        ef_threshold(0.25f) {}

    float pr_ruetz;
    float ng_threshold;
    float ng_release;
    float t2g_length;
    float ef_threshold;

    bool operator==(const Settings &rhs) {
        return Utils::NearlyEqual(pr_ruetz, rhs.pr_ruetz)
            && Utils::NearlyEqual(ng_threshold, rhs.ng_threshold)
            && Utils::NearlyEqual(ng_release, rhs.ng_release)
            && Utils::NearlyEqual(t2g_length, rhs.t2g_length)
            && Utils::NearlyEqual(ef_threshold, rhs.ef_threshold);
    }
    bool operator!=(const Settings &rhs) { return !operator==(rhs); }
};


// Hardware object for the patch_sm
DaisyPatchSM hw;

// Settings
PersistentStorage<Settings> storage(hw.qspi);
Settings default_settings;
bool dosave = false;
bool initialized = false;

// Switch objects
Switch toggle;
Switch button;

// PRat distortion
PRatDist dist;
// PRat noise gate
NoiseGate ng;

// UX values
float satVal = 0.f;


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    static bool first = true;

    static Gate gate;

    static GrabValue<float> cv_gain = 0.f;
    static GrabValue<float> cv_filter = 0.f;
    static GrabValue<float> cv_level = 0.f;
    static GrabValue<float> cv_mix = 0.f;

    static GrabValue<float> ng_threshold = 0.f;
    static GrabValue<float> ng_release = 0.f;
    static GrabValue<float> t2g_length = 0.f;
    static GrabValue<float> ef_threshold = 0.f;

    static float ruetz = 0.f;
    static bool prevshift = false;
    static bool prevtoggle = false;

    hw.ProcessAnalogControls();

    toggle.Debounce();
    button.Debounce();

    // pass-thru until module is initialized
    if (!initialized) {
        Utils::Copy(IN_L, IN_R, OUT_L, OUT_R, size);
        return;
    }

    const bool curtoggle = toggle.Pressed();
    const bool shift = button.Pressed() && !first;

    const float cv1 = hw.GetAdcValue(CV_1);
    const float cv2 = hw.GetAdcValue(CV_2);
    const float cv3 = hw.GetAdcValue(CV_3);
    const float cv4 = hw.GetAdcValue(CV_4);

    if (first) {
        Settings &settings = storage.GetSettings();

        ng_threshold.Update(settings.ng_threshold);
        ng_release.Update(settings.ng_release);
        t2g_length.Update(settings.t2g_length);
        ef_threshold.Update(settings.ef_threshold);
        ruetz = settings.pr_ruetz;

    } else if (!shift && prevshift) { // save settings when exiting shift mode
        Settings &settings = storage.GetSettings();

        settings.ng_threshold = ng_threshold.Get();
        settings.ng_release = ng_release.Get();
        settings.t2g_length = t2g_length.Get();
        settings.ef_threshold = ef_threshold.Get();
        settings.pr_ruetz = ruetz;

        dosave = true;
    }

    if (!shift) {
        cv_gain.Update(cv1);
        cv_filter.Update(cv2);
        cv_level.Update(cv3);
        cv_mix.Update(cv4);

        ng_threshold.Lock();
        ng_release.Lock();
        t2g_length.Lock();
        ef_threshold.Lock();
    } else {
        cv_gain.Lock();
        cv_filter.Lock();
        cv_level.Lock();
        cv_mix.Lock();

        ng_threshold.Update(cv1);
        ng_release.Update(cv2);
        t2g_length.Update(cv3);
        ef_threshold.Update(cv4);
    }

    const float gain = fclamp(cv_gain.Get() + hw.GetAdcValue(CV_6), 0.f, 1.f);
    const float filter = fclamp(cv_filter.Get() + hw.GetAdcValue(CV_7), 0.f, 1.f);
    const float level = fclamp(cv_level.Get() + hw.GetAdcValue(CV_5), 0.f, 1.f);
    const float mix = fclamp(cv_mix.Get() + hw.GetAdcValue(CV_8), 0.f, 1.f);

    const float hard = curtoggle || hw.gate_in_1.State() ? 1.f : 0.f;

    // press shift to enable hard mode with ruetz mod
    if (!prevtoggle && shift && curtoggle) {
        ruetz = 1.f;
    } else if (ruetz >= 0.5f && !curtoggle) {
        Settings &settings = storage.GetSettings();

        settings.pr_ruetz = ruetz = 0.f;

        dosave = true;
    }
    prevtoggle = curtoggle;

    dist.SetParam(PRatDist::P_GAIN, gain);
    dist.SetParam(PRatDist::P_FILTER, filter);
    dist.SetParam(PRatDist::P_LEVEL, level);
    // in hard mode, mix Silicon / Led clippers
    if (hard >= 0.5f) {
        hw.SetLed(true);
        dist.SetParam(PRatDist::P_DRYWET, 1.f);
        dist.SetParam(PRatDist::P_SILED, mix);
    } else {
        hw.SetLed(false);
        dist.SetParam(PRatDist::P_DRYWET, mix);
    }
    dist.SetParam(PRatDist::P_HARD, hard);
    dist.SetParam(PRatDist::P_RUETZ, ruetz);

    dist.Update();

    if (first || shift) {
        gate.SetMinFromParam(t2g_length.Get());

        ng.SetParam(NoiseGate::P_THRESHOLD, ng_threshold.Get());
        ng.SetParam(NoiseGate::P_RELEASE, ng_release.Get());
        // a threshold < -75db disable the noise gate
        ng.SetParam(NoiseGate::P_BYPASS, ng_threshold.Get() < 0.05f ? 1.f : 0.f);

        ng.Update();
    }

    dist.Process(IN_L, IN_R, OUT_L, OUT_R, size);
    // noise gate uses left input for volume detection
    ng.Process(OUT_L, OUT_R, IN_L, OUT_L, OUT_R, size);

    // output the noise gate envelope follower signal (boosted to be in 0-5v range)
    const float env = ng.GetEnvelope() * 2.5f;
    const float envout = fclamp(env * 5.f, 0.f, 5.f);
    hw.WriteCvOut(CV_OUT_1, envout);
    // generate a gate from the envelope follower
    dsy_gpio_write(&hw.gate_out_2, env > ef_threshold.Get());

    // output the distortion saturation
    satVal = fclamp(dist.GetSaturation(), 0.f, 5.f);

    // generate a gate from the input gate (but with a minimum duration)
    gate.Update(hw.gate_in_2.State(), System::GetNow());
    dsy_gpio_write(&hw.gate_out_1, gate.State());

    first = false;
    prevshift = shift;
}


int main(void)
{
    hw.Init();
    //hw.StartLog();
    hw.SetAudioSampleRate(48000);
    hw.SetAudioBlockSize(4);

    storage.Init(default_settings);

    toggle.Init(hw.B8);
    button.Init(hw.B7);

    dist.Init(hw.AudioSampleRate());
    ng.Init(hw.AudioSampleRate());

    // as eurorack audio signals are quite hot, reduce levels a bit before gain stage
    dist.SetParam(PRatDist::P_GAIN_IN, 0.50f);

    ng.SetParam(NoiseGate::P_DETECTOR_GAIN, 0.5);  // * 1
    ng.SetParam(NoiseGate::P_REDUCTION, 0.4);      // -40db
    ng.SetParam(NoiseGate::P_SLOPE, 0.3, true);    // 3

    uint32_t boottime = System::GetNow();

    hw.StartAudio(AudioCallback);

    while (1) {
        if (!initialized) {
            if (System::GetNow() - boottime < 1000) {
                // starting with button pressed restore default settings
                button.Debounce();
                if (button.Pressed()) {
                    Settings &settings = storage.GetSettings();
                    settings = default_settings;
                    dosave = true;
                    // switch led on to confirm settings reset
                    hw.WriteCvOut(CV_OUT_2, 5.f);
                }
            } else {
                initialized = true;
            }
        } else {
            // update led
            hw.WriteCvOut(CV_OUT_2, satVal);
            // save settings
            if (dosave) {
                storage.Save();
                dosave = false;
            }
        }
    }
}
