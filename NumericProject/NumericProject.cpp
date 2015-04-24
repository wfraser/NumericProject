#include <iostream>
#include <cstdint>
#include <sstream>
#include "Number.h"

using namespace std;

int wmain(int argc, wchar_t* argv[])
{
    stringstream out;

    BCD<uint8_t> a(0xFF);
    BCD<uint8_t> b(0xFF);
    BCD<uint8_t> c1 = a + b;
    BCD<uint8_t> c2 = c1.GetAndClearOverflow();

    c2.Print(out);
    c1.Print(out, true);
    assert(out.str() == "510");
    out.str("");

    BCD<uint16_t> x(0xFF7F);
    BCD<uint16_t> y(0xFF7F);
    BCD<uint16_t> z1 = x + y;
    BCD<uint16_t> z2 = z1.GetAndClearOverflow();

    z2.Print(out);
    z1.Print(out, true);
    assert(out.str() == "130814");
    out.str("");

    BigInt<BCD<uint16_t>> xyz(0xFF7F);
    xyz += BCD<uint16_t>(0xFF7F);

    xyz.Print<10>(out);
    assert(out.str() == "130814");
    out.str("");

    return 0;
}
