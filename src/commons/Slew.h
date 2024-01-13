#pragma once
#ifndef PIRAT_SLEW_H
#define PIRAT_SLEW_H

#ifdef __cplusplus

#include <cmath>


namespace pirat
{

struct Slew {
    Slew() {}

    // half time in seconds
    void Init(const float sample_rate, const float htime) {
        htime_ = htime;
        sample_rate_ = sample_rate;
        Update();
    }

    inline void Reset() { state_ = 0.f; }

    inline void Update() {
        c2_ = pow(0.5, 1.0 / sample_rate_ / htime_);
        c1_ = 1.0 - c2_;
    }

    float Process(const float in) {
        state_ = c1_ * in + c2_ * state_;
        return state_;
    }

    private:
    float htime_;
    float sample_rate_;
    float c1_ = 1.f;
    float c2_ = 0.f;
    float state_ = 0.f;
};

}

#endif
#endif
