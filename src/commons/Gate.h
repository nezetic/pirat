#pragma once
#ifndef PIRAT_GATE_H
#define PIRAT_GATE_H

#include "Utils.h"

#ifdef __cplusplus

namespace pirat
{

struct Gate {

    Gate(uint32_t min_ = 12): state(false), prev(false), start(0), min(min_) {}

    inline void Update(bool val, uint32_t now) {
        if (val) {
            if (!prev) {
                start = now;
                state = true;
            }
        } else if (state && now - start > min) {
            state = false;
        }
        prev = val;
    }

    inline bool State() const { return state; };

    inline void SetMin(uint32_t min) {
        this->min = min;
    }

    inline void SetMinFromParam(float min) {
        // 0 -> 10ms
        // 0.1 -> 15ms
        // 0.5 -> 100ms
        // 1 -> 1000ms
        SetMin(static_cast<uint32_t>(Utils::DecResponse(min) * 1000.f));
    }

    private:
    bool state;
    bool prev;
    uint32_t start;
    uint32_t min;
};

}

#endif
#endif
