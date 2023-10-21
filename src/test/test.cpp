#include "PiRATDist.h"
#include "NoiseGate.h"

using namespace pirat;

PiRATDist dist;
NoiseGate ng;


int main() {
    constexpr float fs = 48000.f;

    dist.Init(44100.f);
    dist.SetSampleRate(fs);

    ng.Init(fs);

    ng.SetParam(NoiseGate::P_DETECTOR_GAIN, 0.5);
    ng.SetParam(NoiseGate::P_THRESHOLD, 0.4);
    ng.SetParam(NoiseGate::P_REDUCTION, 0.4);
    ng.SetParam(NoiseGate::P_SLOPE, 0.3);
    ng.SetParam(NoiseGate::P_RELEASE, 0.5, true);

    float signal = 6.66f;

    for (size_t i = 0; i < 16; i++) {
        dist.SetParam(PiRATDist::P_GAIN, 0.32f + (0.1f * i));
        dist.SetParam(PiRATDist::P_FILTER, 0.25f + (0.1f * i));
        dist.SetParam(PiRATDist::P_LEVEL, 0.32f);
        dist.SetParam(PiRATDist::P_DRYWET, 1.f);
        dist.SetParam(PiRATDist::P_HARD, 1.f);

        dist.Update();

        signal = dist.Process(signal);
        signal = ng.Process(signal);
    }

    return 0;
}
