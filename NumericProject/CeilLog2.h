#pragma once

//
// Compile-time constant Ceil(Log2(x)) values :)
//

template <size_t N>
struct CeilLog2
{
    enum { value = 1 + CeilLog2<(N >> 1)>::value };
};

template <>
struct CeilLog2<0>
{
    // Log2(0) is undefined.
};

template <>
struct CeilLog2<1>
{
    enum { value = 1 };
};