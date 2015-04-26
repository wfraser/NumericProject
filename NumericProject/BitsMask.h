#pragma once

//
// Compile-time constant bit mask
//

template <size_t N>
struct BitsMask
{
    enum { value = BitsMask<N - 1>::value << 1 | 1 };
};

template <>
struct BitsMask<0>
{
    enum { value = 0 };
};