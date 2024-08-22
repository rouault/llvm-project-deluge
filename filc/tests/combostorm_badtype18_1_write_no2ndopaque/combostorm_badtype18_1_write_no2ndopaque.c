#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 4) = 42;
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(char**)(buf + 16) = "hello";
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 4);
    int32_t f2 = *(int32_t*)(buf + 8);
    int32_t f3 = *(int32_t*)(buf + 12);
    int32_t f4 = *(int32_t*)(buf + 16);
    int32_t f5 = *(int32_t*)(buf + 20);
    int32_t f6 = *(int32_t*)(buf + 24);
    int32_t f7 = *(int32_t*)(buf + 28);
    int32_t f8 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    return 0;
}
