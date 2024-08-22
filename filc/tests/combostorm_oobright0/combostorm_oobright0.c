#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(33));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 1) = 42;
    *(int8_t*)(buf + 2) = 42;
    *(int8_t*)(buf + 3) = 42;
    *(int8_t*)(buf + 4) = 42;
    *(int8_t*)(buf + 5) = 42;
    *(int8_t*)(buf + 6) = 42;
    *(int8_t*)(buf + 7) = 42;
    *(int8_t*)(buf + 8) = 42;
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 10) = 42;
    *(int8_t*)(buf + 11) = 42;
    *(int8_t*)(buf + 12) = 42;
    *(int8_t*)(buf + 13) = 42;
    *(int8_t*)(buf + 14) = 42;
    *(int8_t*)(buf + 15) = 42;
    *(int8_t*)(buf + 16) = 42;
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 18) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 20) = 42;
    *(int8_t*)(buf + 21) = 42;
    *(int8_t*)(buf + 22) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 24) = 42;
    *(int8_t*)(buf + 25) = 42;
    *(int8_t*)(buf + 26) = 42;
    *(int8_t*)(buf + 27) = 42;
    *(int8_t*)(buf + 28) = 42;
    *(int8_t*)(buf + 29) = 42;
    *(int8_t*)(buf + 30) = 42;
    *(int8_t*)(buf + 31) = 42;
    *(int8_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 26208;
    ZASSERT(*(int8_t*)(buf + 0) == 42);
    ZASSERT(*(int8_t*)(buf + 1) == 42);
    ZASSERT(*(int8_t*)(buf + 2) == 42);
    ZASSERT(*(int8_t*)(buf + 3) == 42);
    ZASSERT(*(int8_t*)(buf + 4) == 42);
    ZASSERT(*(int8_t*)(buf + 5) == 42);
    ZASSERT(*(int8_t*)(buf + 6) == 42);
    ZASSERT(*(int8_t*)(buf + 7) == 42);
    ZASSERT(*(int8_t*)(buf + 8) == 42);
    ZASSERT(*(int8_t*)(buf + 9) == 42);
    ZASSERT(*(int8_t*)(buf + 10) == 42);
    ZASSERT(*(int8_t*)(buf + 11) == 42);
    ZASSERT(*(int8_t*)(buf + 12) == 42);
    ZASSERT(*(int8_t*)(buf + 13) == 42);
    ZASSERT(*(int8_t*)(buf + 14) == 42);
    ZASSERT(*(int8_t*)(buf + 15) == 42);
    ZASSERT(*(int8_t*)(buf + 16) == 42);
    ZASSERT(*(int8_t*)(buf + 17) == 42);
    ZASSERT(*(int8_t*)(buf + 18) == 42);
    ZASSERT(*(int8_t*)(buf + 19) == 42);
    ZASSERT(*(int8_t*)(buf + 20) == 42);
    ZASSERT(*(int8_t*)(buf + 21) == 42);
    ZASSERT(*(int8_t*)(buf + 22) == 42);
    ZASSERT(*(int8_t*)(buf + 23) == 42);
    ZASSERT(*(int8_t*)(buf + 24) == 42);
    ZASSERT(*(int8_t*)(buf + 25) == 42);
    ZASSERT(*(int8_t*)(buf + 26) == 42);
    ZASSERT(*(int8_t*)(buf + 27) == 42);
    ZASSERT(*(int8_t*)(buf + 28) == 42);
    ZASSERT(*(int8_t*)(buf + 29) == 42);
    ZASSERT(*(int8_t*)(buf + 30) == 42);
    ZASSERT(*(int8_t*)(buf + 31) == 42);
    ZASSERT(*(int8_t*)(buf + 32) == 42);
    return 0;
}
