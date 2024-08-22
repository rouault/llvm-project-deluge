#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(int8_t*)(buf + 1) = 42;
    *(int8_t*)(buf + 3) = 42;
    *(int8_t*)(buf + 5) = 42;
    *(int8_t*)(buf + 7) = 42;
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 11) = 42;
    *(int8_t*)(buf + 13) = 42;
    *(int8_t*)(buf + 15) = 42;
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 21) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 25) = 42;
    *(int8_t*)(buf + 27) = 42;
    *(int8_t*)(buf + 29) = 42;
    *(int8_t*)(buf + 31) = 42;
    buf = (char*)(buf) + 26208;
    int8_t f0 = *(int8_t*)(buf + 1);
    int8_t f1 = *(int8_t*)(buf + 3);
    int8_t f2 = *(int8_t*)(buf + 5);
    int8_t f3 = *(int8_t*)(buf + 7);
    int8_t f4 = *(int8_t*)(buf + 9);
    int8_t f5 = *(int8_t*)(buf + 11);
    int8_t f6 = *(int8_t*)(buf + 13);
    int8_t f7 = *(int8_t*)(buf + 15);
    int8_t f8 = *(int8_t*)(buf + 17);
    int8_t f9 = *(int8_t*)(buf + 19);
    int8_t f10 = *(int8_t*)(buf + 21);
    int8_t f11 = *(int8_t*)(buf + 23);
    int8_t f12 = *(int8_t*)(buf + 25);
    int8_t f13 = *(int8_t*)(buf + 27);
    int8_t f14 = *(int8_t*)(buf + 29);
    int8_t f15 = *(int8_t*)(buf + 31);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    return 0;
}
