#pragma once
#ifndef PIRAT_DOUBLECLICKER_H
#define PIRAT_DOUBLECLICKER_H

#include "Utils.h"

#ifdef __cplusplus

namespace pirat
{

struct DoubleClicker {

    DoubleClicker(uint32_t interval = 1000):
        first_(0), last_(0), state_(false), interval_(interval) {}

    inline void Update(bool state, uint32_t now) {
        const bool prev = state_;
        // count a click on a rising edge
        if (state && !prev) {
            // first one is our reference for the interval
            if (first_ == 0) {
                first_ = now;
            } else {
                last_ = now;
            }
        }
        // timeout
        if (first_ && (now > first_ + interval_)) {
            reset();
        }
        state_ = state;
    }

    inline bool DoubleClick() {
        const bool event = last_ != 0;
        // we have detected a double click, reset our state
        if (event) {
            reset();
        }
        return event;
    }

    inline void reset() {
        first_ = 0;
        last_ = 0;
    }

    private:
    uint32_t first_;
    uint32_t last_;

    bool state_;

    const uint32_t interval_;
};

}

#endif
#endif
