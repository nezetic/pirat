#pragma once
#ifndef PRAT_DSP_H
#define PRAT_DSP_H

#ifdef USE_DAISYSP
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wunused-private-field"
#endif // __clang__
#include "daisysp.h"
#pragma GCC diagnostic warning "-Wignored-qualifiers"
#ifdef __clang__
#pragma GCC diagnostic warning "-Wunused-private-field"
#endif // __clang__
#endif // USE_DAISYSP

#ifdef __cplusplus

#include <cmath>

namespace prat
{

class DSP
{
  public:
      DSP() = delete;

      static inline float powf(float f, int n) {
          return std::pow(f, n);
      }

      static inline float powf(float f, float n) {
          return std::pow(f, n);
      }

#ifdef USE_DAISYSP
      static inline float fpowf(float f, int n) {
          return daisysp::fastpower(f, n);
      }

      static inline float pow10f(float n) {
          return daisysp::pow10f(n);
      }

      static inline float fmax(float a, float b) {
          return daisysp::fmax(a, b);
      }

      static inline float fmin(float a, float b) {
          return daisysp::fmin(a, b);
      }

      static inline float fclamp(float in, float min, float max) {
          return daisysp::fclamp(in, min, max);
      }
#else
      static inline float fpowf(float f, int n) {
          return std::powf(f, n);
      }

      static inline float pow10f(float n) {
          return powf(10.0f, n);
      }

      static inline float fmax(float a, float b) {
          return (a > b) ? a : b;
      }

      static inline float fmin(float a, float b) {
          return (a < b) ? a : b;
      }

      static inline float fclamp(float in, float min, float max)
      {
          return fmin(fmax(in, min), max);
      }
#endif
};

}

#endif
#endif
