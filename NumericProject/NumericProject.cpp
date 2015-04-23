#include <iostream>
#include <cstdint>
#include <sstream>
#include "Number.h"

using namespace std;

int wmain(int argc, wchar_t* argv[])
{
    stringstream out;

    BCD<uint16_t> x(0xFF7F);
    BCD<uint16_t> y(0xFF7F);
    BCD<uint16_t> z1 = x + y;
    BCD<uint16_t> z2 = z1.GetOverflow();

    z2.Print(out);
    z1.Print(out, true);

    assert(out.str() == "130814");
    out.str("");

    BigInt<BCD<uint16_t>> xyz(0xFF7F);
    xyz += BCD<uint16_t>(0xFF7F);

    xyz.Print<uint16_t>(out);

    assert(out.str() == "130814");
    out.str("");

    return 0;
}
