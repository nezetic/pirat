#include "NoiseGate.h"
#include "Utils.h"

using namespace pirat;


void NoiseGate::Process(
    const float* inputL,
    const float* inputR,
    const float* detectorInput,
    float* outputL,
    float* outputR,
    size_t len)
{
    Utils::PreventDernormals();

    const bool stereo = inputR != nullptr && outputR != nullptr;

    double gainDb = 1.f;
    double currGain = 0.f;
    double currEnv = 0.f;

    for (size_t i = 0; i < len; i++) {
        const double x = detectorInput[i] * p_detector_gain;
        const double env = envelopeFollower.ProcessEnvelope(x);

        gainDb = expander.Expand(Utils::Gain2DB(env));
        gainDb = slewLimiter.Process(gainDb);

        float gain = Utils::DB2gain(gainDb);

        if (gain > currGain) {
            currGain = gain;
        }
        currEnv = env;

        gain = p_bypass_ > 0.5f ? 1.f : gain;

        outputL[i] = inputL[i] * gain;
        if (stereo) {
            outputR[i] = inputR[i] * gain;
        }
    }

    current_gain_ = currGain;
    current_env_ = currEnv;
}
