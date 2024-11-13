/**
 * Legio implementation of PiRAT distortion, featuring:
 *
 * - stereo signal path;
 * - knobs with dedicated CV controls (either over gain or level);
 * - hard clip, ruetz and tight mods;
 * - noise gate (with a bypass and adjustable threshold / release).
 *
 */
#include "daisy_legio.h"
#include "daisysp.h"

#include "PiRATDist.h"
#include "NoiseGate.h"
#include "DoubleClicker.h"
#include "GrabValue.h"
#include "Slew.h"

using namespace daisy;
using namespace daisysp;

using namespace pirat;

// encoder per-step increment (16 steps from 0 to 1)
#define ENCODER_INCR 0.0625f

// encoder smoothing parameter
#define ENCODER_SMOOTH 0.01f // 10ms

// pitch CV input range is -2 -> 5V
#define PITCH_CV_0V 0.28571428571428575f


enum PitchCVDest {
    GAIN = 0,
    LEVEL = 1
};


struct Settings
{
    Settings():
        pr_gain(0.50f),
        pr_level(0.75f),
        ng_threshold(0.4f),  // -45db
        ng_release(0.5f),    // 100ms
        pitchcvdest(PitchCVDest::GAIN) {}

    float pr_gain;
    float pr_level;
    float ng_threshold;
    float ng_release;

    enum PitchCVDest pitchcvdest;

    bool operator==(const Settings &rhs) {
        return Utils::NearlyEqual(pr_gain, rhs.pr_gain)
            && Utils::NearlyEqual(pr_level, rhs.pr_level)
            && Utils::NearlyEqual(ng_threshold, rhs.ng_threshold)
            && Utils::NearlyEqual(ng_release, rhs.ng_release)
            && (pitchcvdest == rhs.pitchcvdest);
    }
    bool operator!=(const Settings &rhs) { return !operator==(rhs); }
};


// Hardware object for the Legio
DaisyLegio hw;

// Settings
PersistentStorage<Settings> storage(hw.seed.qspi);
Settings default_settings;
bool dosave = false;
bool initialized = false;

// PiRAT distortion
PiRATDist dist;
// PiRAT noise gate
NoiseGate ng;

// smoothing filter for the legio encoder values
Slew sm_gain;
Slew sm_level;

// pitch CV destination selector
enum PitchCVDest pitchcvdest = PitchCVDest::GAIN;
uint32_t last_dck = 0;

// UX values
float envVal = 0.f;
float satVal = 0.f;


static inline float handleIncrement(float value, int32_t inc) {
    if (inc == 1) {
        return fclamp(value + ENCODER_INCR, 0.f, 1.f);
    } else if (inc == -1) {
        return fclamp(value - ENCODER_INCR, 0.f, 1.f);
    }
    return value;
}


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    static bool first = true;
    static bool prevshift = false;

    static float pr_gain = 0.5f;
    static float pr_level = 0.5f;

    static uint32_t last_incr = 0;

    static DoubleClicker dck;

    static GrabValue<float> cv_filter = 0.f;
    static GrabValue<float> cv_mix = 0.f;

    static GrabValue<float> ng_threshold = 0.4f;  // -45db
    static GrabValue<float> ng_release = 0.5f;    // 100ms

    hw.ProcessAllControls();

    // pass-thru until module is initialized
    if (!initialized) {
        Utils::Copy(IN_L, IN_R, OUT_L, OUT_R, size);
        return;
    }

    const uint32_t now = System::GetNow();

    const bool shift = hw.encoder.Pressed() && !first;

    dck.Update(shift, now);
    // double clicking shift switch between Gain and Level CV destinations
    if (dck.DoubleClick()) {
        pitchcvdest = pitchcvdest == PitchCVDest::GAIN ? PitchCVDest::LEVEL : PitchCVDest::GAIN;
        last_dck = now;
    }

    const float cv0 = hw.GetKnobValue(DaisyLegio::CONTROL_KNOB_TOP);
    const float cv1 = hw.GetKnobValue(DaisyLegio::CONTROL_KNOB_BOTTOM);

    if (first) {
        Settings &settings = storage.GetSettings();

        pr_gain = settings.pr_gain;
        pr_level = settings.pr_level;
        ng_threshold.Update(settings.ng_threshold);
        ng_release.Update(settings.ng_release);
        pitchcvdest = settings.pitchcvdest;

    } else if (!shift && prevshift) { // save settings when exiting shift mode
        Settings &settings = storage.GetSettings();

        settings.pr_gain = pr_gain;
        settings.pr_level = pr_level;
        settings.ng_threshold = ng_threshold.Get();
        settings.ng_release = ng_release.Get();
        settings.pitchcvdest = pitchcvdest;

        // force editing mode to timeout
        last_incr = 0;
        dosave = true;
    }

    prevshift = shift;

    const int32_t inc = hw.encoder.Increment();

    // encoder is in editing mode
    if (inc != 0) {
        last_incr = now;
    }

    if (!shift) {
        pr_gain = handleIncrement(pr_gain, inc);

        cv_filter.Update(cv0);
        cv_mix.Update(cv1);

        ng_threshold.Lock();
        ng_release.Lock();
    } else {
        pr_level = handleIncrement(pr_level, inc);

        cv_filter.Lock();
        cv_mix.Lock();

        ng_threshold.Update(cv0);
        ng_release.Update(cv1);
    }

    // smooth encoder values
    const float pr_gain_sm = sm_gain.Process(pr_gain);
    const float pr_level_sm = sm_level.Process(pr_level);

    const float pitch_cv = hw.controls[DaisyLegio::CONTROL_PITCH].Value();
    // pitch CV range is -2V -> 5V, we want to remap it in range 0-5V
    const float pitch_cv_0v = fclamp((pitch_cv - PITCH_CV_0V) / (1.0f - PITCH_CV_0V), 0.f, 1.f);

    float gain_cv = 0.f;
    float level_cv = 0.f;

    if (pitchcvdest == PitchCVDest::GAIN) {
        gain_cv = pitch_cv_0v;
    } else {
        level_cv = pitch_cv_0v;
    }

    const float gain = fclamp(pr_gain_sm + gain_cv, 0.f, 1.f);
    const float filter = fclamp(cv_filter.Get(), 0.f, 1.f);
    const float level = fclamp(pr_level_sm + level_cv, 0.f, 1.f);
    const float mix = fclamp(cv_mix.Get(), 0.f, 1.f);

    const int sw_clip = hw.sw[DaisyLegio::SW_LEFT].Read();
    const int sw_mod = hw.sw[DaisyLegio::SW_RIGHT].Read();

    const bool gate = !hw.Gate(); // gate is inverted
    const float hard = (sw_clip == Switch3::POS_CENTER) || gate ? 1.f : 0.f;
    const float bypass = sw_clip == Switch3::POS_DOWN ? 1.f : 0.f;
    const float ruetz = sw_mod == Switch3::POS_CENTER ? 1.f : 0.f;
    const float tight = sw_mod == Switch3::POS_DOWN ? 1.f : 0.f;

    dist.SetParam(PiRATDist::P_GAIN, gain);
    dist.SetParam(PiRATDist::P_FILTER, filter);
    dist.SetParam(PiRATDist::P_LEVEL, level);
    // in hard mode, mix Silicon / Led clippers
    if (hard >= 0.5f) {
        dist.SetParam(PiRATDist::P_DRYWET, 1.f);
        dist.SetParam(PiRATDist::P_SILED, mix);
    } else {
        dist.SetParam(PiRATDist::P_DRYWET, mix);
    }
    dist.SetParam(PiRATDist::P_HARD, hard);
    dist.SetParam(PiRATDist::P_TIGHT, tight);
    dist.SetParam(PiRATDist::P_RUETZ, ruetz);
    dist.SetParam(PiRATDist::P_BYPASS, bypass);

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

    // encoder is in editing mode
    if (last_incr != 0 && (now - last_incr) < 1000) {
        if (shift) {
            satVal = 0.f;
            envVal = pr_level_sm;
        } else {
            satVal = pr_gain_sm;
            envVal = 0.f;
        }
    } else {
        if (last_incr != 0) {
            // force settings to be saved when leaving encoder editing mode
            prevshift = true;
        }
        last_incr = 0;

        // output the distortion saturation
        satVal = fclamp(dist.GetSaturation() / 5.f, 0.f, 1.f);
        // noise gate envelope follower signal (boosted to be in 0-1v range)
        envVal = fclamp(ng.GetEnvelope() * 2.5f, 0.f, 1.f);
    }

    first = false;
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

    sm_gain.Init(sr, ENCODER_SMOOTH);
    sm_level.Init(sr, ENCODER_SMOOTH);

    // as eurorack audio signals are quite hot, reduce levels a bit before gain stage
    dist.SetParam(PiRATDist::P_GAIN_IN, 0.50f);

    ng.SetParam(NoiseGate::P_DETECTOR_GAIN, 0.5);  // * 1
    ng.SetParam(NoiseGate::P_REDUCTION, 0.4);      // -40db
    ng.SetParam(NoiseGate::P_SLOPE, 0.3, true);    // 3

    uint32_t boottime = System::GetNow();

    hw.StartAudio(AudioCallback);
    hw.StartAdc();

    while (1) {
        if (!initialized) {
            if (System::GetNow() - boottime < 1000) {
                hw.ProcessDigitalControls();
                // starting with button pressed restore default settings
                if (hw.encoder.Pressed()) {
                    Settings &settings = storage.GetSettings();
                    settings = default_settings;
                    dosave = true;
                }
                if (first) {
                    hw.SetLed(DaisyLegio::LED_LEFT, 1.f, 0.f, 0.f);
                    hw.SetLed(DaisyLegio::LED_RIGHT, 1.f, 0.f, dosave ? 0.f : 1.f);
                    hw.UpdateLeds();

                    first = false;
                }
            } else {
                initialized = true;
            }
        } else {
            // indicate current pitch CV destination
            if (System::GetNow() - last_dck < 1000) {
                hw.SetLed(DaisyLegio::LED_LEFT, 0.f, 0.f, pitchcvdest == PitchCVDest::LEVEL ? 1.f : 0.f);
                hw.SetLed(DaisyLegio::LED_RIGHT, 0.f, 0.f, pitchcvdest == PitchCVDest::GAIN ? 1.f : 0.f);
            } else {
                hw.SetLed(DaisyLegio::LED_LEFT, 0.f, envVal, 0.f);
                hw.SetLed(DaisyLegio::LED_RIGHT, satVal, 0.f, 0.f);
            }
            hw.UpdateLeds();
            // save settings
            if (dosave) {
                storage.Save();
                dosave = false;
            }
        }
    }
}
