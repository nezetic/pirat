#pragma once
#ifndef PIRAT_GRABVALUE_H
#define PIRAT_GRABVALUE_H

#ifdef __cplusplus

namespace pirat
{

enum GrabDir {
    NONE,
    UP,
    DOWN
};


template<typename T>
struct GrabValue {
    GrabValue(const T init):
        locked_(false),
        direction_(GrabDir::NONE),
        value_(init) {}

    inline void Update(const T in) {
        if (locked_) {
            if (fabsf(in - value_) > 0.1f) {
                direction_ = in < value_ ? GrabDir::UP : GrabDir::DOWN;
            }
            locked_ = false;
        }
        if ((direction_ == GrabDir::UP && in >= value_) ||
            (direction_ == GrabDir::DOWN && in <= value_)) {
            direction_ = GrabDir::NONE;
        }
        // only update value if we are not catching up
        if (direction_ == GrabDir::NONE) {
            value_ = in;
        }
    }

    inline void Lock() { locked_ = true; }

    inline T Get() const { return value_; }

    private:
    bool locked_;
    enum GrabDir direction_;
    T value_;
};

}

#endif
#endif
