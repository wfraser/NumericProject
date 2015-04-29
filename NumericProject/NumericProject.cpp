#include <iostream>
#include <cstdint>
#include <sstream>

#include "BigInt.h"

using namespace std;

int main(int argc, char* argv[])
{
#if 0
    {
        // This would fail because it will overflow the overflow segment.
        // It involves doing 100 * 5 as the last step, which is too big for a uint8_t.
        // I think multiplication on overflowed numbers should be forbidden.
        BCD<uint8_t> aa(0xFF);
        BCD<uint8_t> bb(0xFF);
        BCD<uint8_t> cc = aa * bb;
    }
#endif

    // Test on BCD<uint8_t>
    {
        stringstream out;

        BCD<uint8_t> a(0xFF);
        BCD<uint8_t> b(0xFF);
        BCD<uint8_t> c1 = a + b;
        BCD<uint8_t> c2 = c1.GetAndClearOverflow();

        c2.Print(out);
        c1.Print(out, true);
        assert(out.str() == "510");
    }

    // Test on BCD<uint16_t>
    {
        stringstream out;

        BCD<uint16_t> a(0xFF7F);
        BCD<uint16_t> b(0xFF7F);
        BCD<uint16_t> c1 = a + b;
        BCD<uint16_t> c2 = c1.GetAndClearOverflow();

        c2.Print(out);
        c1.Print(out, true);
        assert(out.str() == "130814");
    }

    // Test printing a BigInt<BCD>
    {
        stringstream out;

        BigInt<BCD<uint16_t>> xyz(0xFF7F);
        xyz += BCD<uint16_t>(0xFF7F);

        xyz.Print<10>(out);
        assert(out.str() == "130814");
    }

    // Test printing a BigInt<uint16_t>
    // Internally it converts to a BigInt<BCD<uint16_t, 10>> for printing.
    {
        stringstream out;

        BigInt<uint16_t> printTest(0xFF7F);
        printTest += 0xFF7F;

        printTest.Print<10>(out);
        assert(out.str() == "130814");
    }

    cout << "Done\n";
    return 0;
}
