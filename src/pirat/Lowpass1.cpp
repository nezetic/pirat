/**
 * Ported from Valdemar Erlingsson original C# implementation.
 *
 * https://github.com/ValdemarOrn/SharpSoundPlugins/tree/master/Rodent.V2
 */
#include "Lowpass1.h"

#include <cmath>

using namespace pirat;


void Lowpass1::Init(float sample_rate) {
    sample_rate_ = sample_rate;
    Transfer::Init();
}


void Lowpass1::Update() {
    std::array<float, 2> b = {0.f, 0.f};
    std::array<float, 2> a = {0.f, 0.f};

    // PRevent going over the Nyquist frequency
    if(freq_ >= sample_rate_ * 0.5) {
        freq_ = sample_rate_ * 0.499;
    }

    // Compensate for frequency in bilinear transform
    float f = (float)(2.0 * sample_rate_ * (std::tan((freq_ * 2 * M_PI) / (sample_rate_ * 2))));

    if (f == 0) f = 0.0001f; // prevent divByZero exception

    b[0] = f;
    b[1] = f;

    a[0] = f+2*sample_rate_;
    a[1] = f-2*sample_rate_;

    float aInv = 1 / a[0];

    // normalize
    b[0] = b[0] * aInv;
    b[1] = b[1] * aInv;

    a[1] = a[1] * aInv;
    a[0] = 1;

    SetB(b);
    SetA(a);
}

