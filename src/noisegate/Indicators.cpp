/**
 * Based on the amazing NoiseInvaderVST from ValdemarOrn.
 *
 * https://github.com/ValdemarOrn/NoiseInvaderVST
 */
#include "Indicators.h"
#include "Utils.h"


using namespace prat;


void Sma::Init(size_t sampleCount) {
    sampleCount = std::min(sampleCount, queue.size());
    this->sampleCount = sampleCount;
    for (size_t i = 0; i < sampleCount; i++)
        queue[i] = 0.0;

    head = 0;
    sum = 0.0;
    dbDecayPerSample = 0.0;
}


double Sma::Update(double sample) {
    double takeAway = queue[head];
    queue[head] = sample;
    head++;
    if (head >= sampleCount)
        head = 0;

    sum -= takeAway;
    sum += sample;

    float sampleDb = Utils::Gain2DB(sample);
    float takeAwayDb = Utils::Gain2DB(takeAway);

    if (sampleDb < -150)
        sampleDb = -150;
    if (takeAwayDb < -150)
        takeAwayDb = -150;

    dbDecayPerSample = (sampleDb - takeAwayDb) / sampleCount;

    return sum / sampleCount;
}
