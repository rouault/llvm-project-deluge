#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(48));
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
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int16_t*)(buf + 0) == 42);
    ZASSERT(*(int16_t*)(buf + 2) == 42);
    ZASSERT(*(int16_t*)(buf + 4) == 42);
    ZASSERT(*(int16_t*)(buf + 6) == 42);
    ZASSERT(*(int16_t*)(buf + 8) == 42);
    ZASSERT(*(int16_t*)(buf + 10) == 42);
    ZASSERT(*(int16_t*)(buf + 12) == 42);
    ZASSERT(*(int16_t*)(buf + 14) == 42);
    ZASSERT(*(int16_t*)(buf + 16) == 42);
    ZASSERT(*(int16_t*)(buf + 18) == 42);
    ZASSERT(*(int16_t*)(buf + 20) == 42);
    ZASSERT(*(int16_t*)(buf + 22) == 42);
    ZASSERT(*(int16_t*)(buf + 24) == 42);
    ZASSERT(*(int16_t*)(buf + 26) == 42);
    ZASSERT(*(int16_t*)(buf + 28) == 42);
    ZASSERT(*(int16_t*)(buf + 30) == 42);
    ZASSERT(*(int16_t*)(buf + 32) == 42);
    return 0;
}
