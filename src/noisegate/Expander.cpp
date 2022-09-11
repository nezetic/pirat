/**
 * Based on the amazing NoiseInvaderVST from ValdemarOrn.
 *
 * https://github.com/ValdemarOrn/NoiseInvaderVST
 */
#include "Expander.h"
#include "Utils.h"


using namespace prat;


double Expander::Expand(double dbVal) {
    if (std::isnan(outputDb) || std::isinf(outputDb)) {
        outputDb = -150;
    }

    // 1. The two expansion curve form the upper and lower boundary of what the permitted "desired dB" value will be
    double upperDb = Compress(dbVal, thresholdDb, upperSlope, 4, true);
    double lowerDb = Compress(dbVal, thresholdDb + 4, lowerSlope, 4, true);

    // 2. The status quo, if neither curve is "hit", is to increase or reduce the desired dB by the 
    // change in input dB. This change is applied to whatever the current output dB currently is.
    double dbChange = dbVal - prevInDb;
    double desiredDb = outputDb + dbChange;

    // 3. apply the upper and lower curves as limits to the desired dB
    if (desiredDb < lowerDb) {
        desiredDb = lowerDb;
    } else if (desiredDb > upperDb) {
        desiredDb = upperDb;
    }

    outputDb = desiredDb;
    prevInDb = dbVal;

    // 4. do not let the effective gain go below the reductiondB value
    double gainDiff = outputDb - dbVal;
    if (gainDiff < reductionDb) {
        gainDiff = reductionDb;
    }

    return gainDiff;
}


double Expander::Compress(double x, double threshold, double ratio, double knee, bool expand) {
    // the assumed gain
    double output;
    double kneeLow = threshold - knee;
    double kneeHigh = threshold + knee;

    if (x <= kneeLow) {
        output = x;
    } else if (x >= kneeHigh) {
        double diff = x - threshold;
        double a = threshold + diff / ratio;
        output = a;
    } else { // in knee, below threshold
        // position on the interpolating line between the two parts of the compression curve
        double positionOnLine = (x - kneeLow) / (kneeHigh - kneeLow);
        double kDiff = knee * positionOnLine;
        double xa = kneeLow + kDiff;
        double ya = xa;
        double yb = threshold + kDiff / ratio;
        double slope = (yb - ya) / (knee);
        output = xa + slope * positionOnLine * knee;
    }

    // if doing expansion, adjust the slopes so that instead of compressing, the curve expands
    if (expand) {
        // to expand, we multiple the output by the amount of the rate. this way, the upper portion of the curve has slope 1.
        // we then add a y-offset to reset the threshold back to the original value
        double modifiedThrehold = threshold * ratio;
        double yOffset = modifiedThrehold - threshold;
        output = output * ratio - yOffset;
    }

    return output;
}
