#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"
static char* hello = "hello";
static unsigned char value;
static void init_test(void)
{
    unsigned index;
    value = 42;
    bool good = false;
    while (!good) {
        good = true;
        for (index = sizeof(char*); index--;) {
            if (((char*)&hello)[index] == value) {
                good = false;
                break;
            }
        }
        if (good)
            break;
        value++;
    }
}
int main()
{
    init_test();
    char* buf = opaque(malloc(25));
    *(int8_t*)(buf + 0) = value;
    *(int8_t*)(buf + 1) = value;
    *(int8_t*)(buf + 2) = value;
    *(int8_t*)(buf + 3) = value;
    *(int8_t*)(buf + 4) = value;
    *(int8_t*)(buf + 5) = value;
    *(int8_t*)(buf + 6) = value;
    *(int8_t*)(buf + 7) = value;
    *(int8_t*)(buf + 8) = value;
    *(int8_t*)(buf + 9) = value;
    *(int8_t*)(buf + 10) = value;
    *(int8_t*)(buf + 11) = value;
    *(int8_t*)(buf + 12) = value;
    *(int8_t*)(buf + 13) = value;
    *(int8_t*)(buf + 14) = value;
    *(int8_t*)(buf + 15) = value;
    *(int8_t*)(buf + 16) = value;
    *(int8_t*)(buf + 17) = value;
    *(int8_t*)(buf + 18) = value;
    *(int8_t*)(buf + 19) = value;
    *(int8_t*)(buf + 20) = value;
    *(int8_t*)(buf + 21) = value;
    *(int8_t*)(buf + 22) = value;
    *(int8_t*)(buf + 23) = value;
    *(int8_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + -26208;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 1);
    int8_t f2 = *(int8_t*)(buf + 2);
    int8_t f3 = *(int8_t*)(buf + 3);
    int8_t f4 = *(int8_t*)(buf + 4);
    int8_t f5 = *(int8_t*)(buf + 5);
    int8_t f6 = *(int8_t*)(buf + 6);
    int8_t f7 = *(int8_t*)(buf + 7);
    int8_t f8 = *(int8_t*)(buf + 8);
    int8_t f9 = *(int8_t*)(buf + 9);
    int8_t f10 = *(int8_t*)(buf + 10);
    int8_t f11 = *(int8_t*)(buf + 11);
    int8_t f12 = *(int8_t*)(buf + 12);
    int8_t f13 = *(int8_t*)(buf + 13);
    int8_t f14 = *(int8_t*)(buf + 14);
    int8_t f15 = *(int8_t*)(buf + 15);
    int8_t f16 = *(int8_t*)(buf + 16);
    int8_t f17 = *(int8_t*)(buf + 17);
    int8_t f18 = *(int8_t*)(buf + 18);
    int8_t f19 = *(int8_t*)(buf + 19);
    int8_t f20 = *(int8_t*)(buf + 20);
    int8_t f21 = *(int8_t*)(buf + 21);
    int8_t f22 = *(int8_t*)(buf + 22);
    int8_t f23 = *(int8_t*)(buf + 23);
    int8_t f24 = *(int8_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
    ZASSERT(f6 == value);
    ZASSERT(f7 == value);
    ZASSERT(f8 == value);
    ZASSERT(f9 == value);
    ZASSERT(f10 == value);
    ZASSERT(f11 == value);
    ZASSERT(f12 == value);
    ZASSERT(f13 == value);
    ZASSERT(f14 == value);
    ZASSERT(f15 == value);
    ZASSERT(f16 == value);
    ZASSERT(f17 == value);
    ZASSERT(f18 == value);
    ZASSERT(f19 == value);
    ZASSERT(f20 == value);
    ZASSERT(f21 == value);
    ZASSERT(f22 == value);
    ZASSERT(f23 == value);
    ZASSERT(f24 == value);
    return 0;
}
