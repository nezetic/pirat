#pragma once
#ifndef PRAT_SPLINE_H
#define PRAT_SPLINE_H

#ifdef __cplusplus

#include <cstddef>
#include <array>

namespace prat
{

class Splines
{
    public:

        static constexpr size_t D1N914TF_len = 10;
        static constexpr size_t LEDTF_len = 11;

        static const std::array<std::array<float, D1N914TF_len>, 3> D1N914TF;
        static const std::array<std::array<float, LEDTF_len>, 3> LEDTF;
};

} // namespace prat
#endif
#endif
