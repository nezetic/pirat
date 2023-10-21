/**
 * Based on the amazing NoiseInvaderVST from ValdemarOrn.
 *
 * https://github.com/ValdemarOrn/NoiseInvaderVST
 */
#include "EnvelopeFollower.h"
#include "Utils.h"


using namespace pirat;


void EnvelopeFollower::Init(double fs, double releaseMs) {
    Fs = fs;
    double ts = 1.0 / fs;

    hpFilter.SetFc(InputFilterHpCutoff / (fs * 0.5));

    inputFilter.Init(Biquad::FilterType::LowPass, fs);
    inputFilter.SetFreq(InputFilterCutoff);
    inputFilter.SetQ(1.0f);
    inputFilter.Update();
    
    double emaAlpha = Utils::ComputeLpAlpha(EmaFc, ts);

    double slowDbDecayPerSample = -60 / (3000 / 1000.0 * fs);
    slowDecay = Utils::DB2gain(slowDbDecayPerSample);

    SetRelease(releaseMs);

    sma.Init((int)(fs * SmaPeriodSeconds));
    ema.Init(emaAlpha);
    movementLatch.Init(0.005, 0.2); // frequency dependent, but not really that critical...

    triggerCounterTimeoutSamples = (int)(fs * TimeoutPeriodSeconds);

    holdAlpha = Utils::ComputeLpAlpha(HoldSmootherFc, ts);
}


double EnvelopeFollower::ProcessEnvelope(double val) {
    double combinedFiltered;
    double decay;

    // 1. Rectify the input signal
    val = std::abs(val);

    // 2. Band pass filter to ~  100hz - 2Khz
    val = hpFilter.Process(val);
    float lpValue = inputFilter.Process(val);
    //hpSignal = hipassAlpha * lpSignal + (1 - hipassAlpha) * hpSignal

    // rectify the lpValue again, because the resonance in the filter can cause a tiny bit of ringing and cause the values to go negative again
    lpValue = std::abs(lpValue);

    float mainInput = lpValue;

    // 3. Compute the EMA and SMA of the band-filtered signal. Also compute the per-sample dB decay baed on the SMA
    float emaValue = ema.Update(mainInput);
    float smaValue = sma.Update(mainInput);

    // 4. use a latching low-pass classifier to determine if signal strength is generally increasing or decreasing.
    // This removes spike from the signal where the SMA may move in the opposite direction for a short period
    float movementValue = movementLatch.Update(sma.GetDbDecayPerSample() > 0);

    // 5. If the movement is going up, prefer the faster moving EMA signal if it's above the SMA
    // If the movement is going down, prefer the faster moving EMA signal if it's below the SMA
    // otherwise, use SMA
    if (movementValue > 0) // going up
        combinedFiltered = emaValue > smaValue ? emaValue : smaValue;
    else // going down
        combinedFiltered = emaValue < smaValue ? emaValue : smaValue;

    // 6. use a hold mechanism to store the peak
    if (combinedFiltered > hold)
    {
        hold = combinedFiltered;
        lastTriggerCounter = 0;
    }

    // 7. Choosing the decay speed
    // Under normal conditions, use the decay from the SMA, scaled by a fudge factor to make it slightly faster decaying.
    // The reason for this is so that we gently bump into the peaks of the signal once in a while.
    // If the hold mechanism hasn't been triggered for a specific timeout, then the current hold value is too high, and we need to rapidly decay downwards.
    // Use the fastDecay (based on the user- specified release value) as a slew limited value
    if (lastTriggerCounter > triggerCounterTimeoutSamples)
        decay = fastDecay;
    else
        decay = Utils::DB2gain(sma.GetDbDecayPerSample() * 1.2); // 1.2 is fudge factor to make the follower decay slightly faster than actual signal, so we gently bump into the peaks

    // 7.5 Limit the decay speed in the general range of slowDecay...fastDecay, the slow decay is currently a fixed 3 seconds to -60dB value
    if (decay > slowDecay)
        decay = slowDecay;
    if (decay < fastDecay)
        decay = fastDecay;

    hold = hold * decay;

    // 8. Filter the resulting hold signal to retrieve a smooth envelope.
    // Currently using 4x 1 pole lowpass, should replace with a proper 4th order butterworth
    h1 = holdAlpha * hold + (1 - holdAlpha) * h1;
    h2 = holdAlpha * h1 + (1 - holdAlpha) * h2;
    h3 = holdAlpha * h2 + (1 - holdAlpha) * h3;
    h4 = holdAlpha * h3 + (1 - holdAlpha) * h4;

    lastTriggerCounter++;
    return h4;
}
