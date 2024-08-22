#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(34));
    *(char**)(buf + 0) = "hello";
    *(int16_t*)(buf + 16) = 42;
    *(int16_t*)(buf + 24) = 42;
    *(int16_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 8);
    int16_t f2 = *(int16_t*)(buf + 16);
    int16_t f3 = *(int16_t*)(buf + 24);
    int16_t f4 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    return 0;
}
