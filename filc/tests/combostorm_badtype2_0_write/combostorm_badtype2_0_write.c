#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 21) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 25) = 42;
    *(int8_t*)(buf + 27) = 42;
    *(int8_t*)(buf + 29) = 42;
    *(int8_t*)(buf + 31) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 1) == 42);
    ZASSERT(*(int8_t*)(buf + 3) == 42);
    ZASSERT(*(int8_t*)(buf + 5) == 42);
    ZASSERT(*(int8_t*)(buf + 7) == 42);
    ZASSERT(*(int8_t*)(buf + 9) == 42);
    ZASSERT(*(int8_t*)(buf + 11) == 42);
    ZASSERT(*(int8_t*)(buf + 13) == 42);
    ZASSERT(*(int8_t*)(buf + 15) == 42);
    ZASSERT(*(int8_t*)(buf + 17) == 42);
    ZASSERT(*(int8_t*)(buf + 19) == 42);
    ZASSERT(*(int8_t*)(buf + 21) == 42);
    ZASSERT(*(int8_t*)(buf + 23) == 42);
    ZASSERT(*(int8_t*)(buf + 25) == 42);
    ZASSERT(*(int8_t*)(buf + 27) == 42);
    ZASSERT(*(int8_t*)(buf + 29) == 42);
    ZASSERT(*(int8_t*)(buf + 31) == 42);
    return 0;
}
