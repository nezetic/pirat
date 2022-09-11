/*
 * Based on the amazing NoiseInvaderVST from ValdemarOrn.
 *
 * https://github.com/ValdemarOrn/NoiseInvaderVST
 */
#pragma once

#ifndef PRAT_ONEPOLEFILTERS
#define PRAT_ONEPOLEFILTERS

#ifdef __cplusplus

#include <cmath>

namespace prat
{
    class Lp1
    {
    private:

        float z1_state;
        //float g;
        float g2;

    public:

        inline float Process(float x) {
            // perform one sample tick of the lowpass filter
            //float v = (x - z1_state) * g / (1 + g);
            float v = (x - z1_state) * g2;
            float y = v + z1_state;
            z1_state = y + v;
            return y;
        }

        // 0...1
        inline void SetFc(float fcRel)
        {
            //this->g = fcRel * M_PI;
            float g = (float)(fcRel * M_PI);
            g2 = g / (1 + g);
        }
    };

    class Hp1
    {
    private:

        float z1_state;
        //float g;
        float g2;

    public:

        inline float Process(float x)
        {
            // perform one sample tick of the lowpass filter
            //float v = (x - z1_state) * g / (1 + g);
            float v = (x - z1_state) * g2;
            float y = v + z1_state;
            z1_state = y + v;
            return x - y;
        }

        // 0...1
        inline void SetFc(float fcRel)
        {
            //this->g = fcRel * M_PI;
            float g = (float)(fcRel * M_PI);
            g2 = g / (1 + g);
        }
    };
}

#endif
#endif // PRAT_ONEPOLEFILTERS
