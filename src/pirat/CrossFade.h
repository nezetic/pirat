#pragma once

#ifndef PIRAT_CROSSFADE_H
#define PIRAT_CROSSFADE_H

#include "DSP.h"

#ifdef __cplusplus

namespace pirat
{
#ifdef USE_DAISYSP
    class CrossFade: public daisysp::CrossFade {
        public:

        void Process(
            const float* inputL,
            const float* inputR,
            const float* input2L,
            const float* input2R,
            float* outputL,
            float* outputR,
            size_t len) {

            const bool stereo = inputR != nullptr && input2R != nullptr && outputR != nullptr;
            for (size_t i = 0; i < len; i++) {
                float A = inputL[i];
                float B = input2L[i];
                outputL[i] = this->daisysp::CrossFade::Process(A, B);
                if (stereo) {
                    A = inputR[i];
                    B = input2R[i];
                    outputR[i] = this->daisysp::CrossFade::Process(A, B);
                }
            }
        }

        inline float Process(const float in1, const float in2) {
            float out = 0.f;
            Process(&in1, nullptr, &in2, nullptr, &out, nullptr, 1);
            return out;
        }
    };
#endif
}

#endif
#endif // PIRAT_CROSSFADE_H
