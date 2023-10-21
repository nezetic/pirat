/**
 * Ported from Valdemar Erlingsson original C# implementation.
 *
 * https://github.com/ValdemarOrn/SharpSoundPlugins/tree/master/Rodent.V2
 */
#include "TFGain.h"
#include "Bilinear.h"


using namespace pirat;


void TFGain::Init(float sample_rate) {
    sample_rate_ = sample_rate;
    Transfer::Init();
}


void TFGain::Update() {
    float R1 = 560;
    float R2 = 47 + (ruetz_ ? 10000 : 0); // ~ infinite resistance if ruetz = 1
    // note, R2 can't go too big since it causes a massive DC offset spike.
    float C1 = 4.7e-6;
    float C2 = 2.2e-6;
    float Gain = 150e3 * gain_;
    float C3 = 100e-12;

    std::array<float, 4> sb = { 1.0f, (C1 * Gain + C2 * Gain + C3 * Gain + C1 * R1 + C2 * R2), (C1 * C2 * Gain * R1 + C1 * C3 * Gain * R1 + C1 * C2 * Gain * R2 + C2 * C3 * Gain * R2 + C1 * C2 * R1 * R2), (C1 * C2 * C3 * Gain * R1 * R2) };
    std::array<float, 4> sa = { 1.0f, (C3 * Gain + C1 * R1 + C2 * R2), (C1 * C3 * Gain * R1 + C2 * C3 * Gain * R2 + C1 * C2 * R1 * R2), (C1 * C2 * C3 * Gain * R1 * R2) };

    std::array<float, 4> zb, za;

    Bilinear<4>::Transform(sb, sa, zb, za, sample_rate_);

    SetB(zb);
    SetA(za);
}

