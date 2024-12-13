#include <stdfil.h>

struct moreThan64BoolOne
{
    bool regular_bool;

    bool a00: 1;
    bool a01: 1;
    bool a02: 1;
    bool a03: 1;
    bool a04: 1;
    bool a05: 1;
    bool a06: 1;
    bool a07: 1;
    bool a08: 1;
    bool a09: 1;
    bool a10: 1;
    bool a11: 1;
    bool a12: 1;
    bool a13: 1;
    bool a14: 1;
    bool a15: 1;
    bool a16: 1;
    bool a17: 1;
    bool a18: 1;
    bool a19: 1;
    bool a20: 1;
    bool a21: 1;
    bool a22: 1;
    bool a23: 1;
    bool a24: 1;
    bool a25: 1;
    bool a26: 1;
    bool a27: 1;
    bool a28: 1;
    bool a29: 1;
    bool a30: 1;
    bool a31: 1;
    bool a32: 1;
    bool a33: 1;
    bool a34: 1;
    bool a35: 1;
    bool a36: 1;
    bool a37: 1;
    bool a38: 1;
    bool a39: 1;
    bool a40: 1;
    bool a41: 1;
    bool a42: 1;
    bool a43: 1;
    bool a44: 1;
    bool a45: 1;
    bool a46: 1;
    bool a47: 1;
    bool a48: 1;
    bool a49: 1;
    bool a50: 1;
    bool a51: 1;
    bool a52: 1;
    bool a53: 1;
    bool a54: 1;
    bool a55: 1;
    bool a56: 1;
    bool a57: 1;
    bool a58: 1;
    bool a59: 1;
    bool a60: 1;
    bool a61: 1;
    bool a62: 1;
    bool a63: 1;

    bool a64: 1;

    moreThan64BoolOne(bool x, bool y): a00(x), a64(y) {}
};

void test3(bool x, bool y)
{
    moreThan64BoolOne lots(x, y);
    ZASSERT(lots.a00 == x);
    ZASSERT(lots.a64 == y);
}

int main()
{
    test3(true, true);
    test3(false, true);
    test3(true, false);
    test3(false, false);
    return 0;
}
