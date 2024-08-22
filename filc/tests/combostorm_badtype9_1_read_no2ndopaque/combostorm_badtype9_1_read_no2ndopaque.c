#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(34));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 2) = 42;
    *(int16_t*)(buf + 4) = 42;
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 8) = 42;
    *(int16_t*)(buf + 10) = 42;
    *(int16_t*)(buf + 12) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 16) = 42;
    *(int16_t*)(buf + 18) = 42;
    *(int16_t*)(buf + 20) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 24) = 42;
    *(int16_t*)(buf + 26) = 42;
    *(int16_t*)(buf + 28) = 42;
    *(int16_t*)(buf + 30) = 42;
    *(int16_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 2);
    int16_t f2 = *(int16_t*)(buf + 4);
    int16_t f3 = *(int16_t*)(buf + 6);
    int16_t f4 = *(int16_t*)(buf + 8);
    int16_t f5 = *(int16_t*)(buf + 10);
    int16_t f6 = *(int16_t*)(buf + 12);
    int16_t f7 = *(int16_t*)(buf + 14);
    char* f8 = *(char**)(buf + 16);
    int16_t f9 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(!strcmp(f8, "hello"));
    ZASSERT(f9 == 42);
    return 0;
}
