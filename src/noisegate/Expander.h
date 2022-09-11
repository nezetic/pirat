/**
 * Based on the amazing NoiseInvaderVST from ValdemarOrn.
 *
 * https://github.com/ValdemarOrn/NoiseInvaderVST
 */
#pragma once

#ifndef PRAT_EXPANDER_H
#define PRAT_EXPANDER_H

#ifdef __cplusplus

#include <cmath>

namespace prat
{
    class Expander
    {
    private:

        double prevInDb = -150.0;
        double outputDb = -150.0;

        double reductionDb;
        double upperSlope;
        double lowerSlope;
        double thresholdDb;

    public:
        Expander()
        {
            Update(-20, -100, 2.0);
        }

        void Update(double thresholdDb, double reductionDb, double slope) {
            this->thresholdDb = thresholdDb;
            this->reductionDb = reductionDb;
            upperSlope = slope;
            lowerSlope = slope * 2;
        }

        double Expand(double dbVal);

    private:

        /// <summary>
        /// Given an input dB value, will compress or expand it according to the parameters specified
        /// </summary>
        double Compress(double x, double threshold, double ratio, double knee, bool expand);

    };
}

#endif
#endif // PRAT_EXPANDER_H
