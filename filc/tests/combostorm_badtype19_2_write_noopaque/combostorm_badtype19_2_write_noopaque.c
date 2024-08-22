#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(48));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 8);
    int32_t f2 = *(int32_t*)(buf + 16);
    int32_t f3 = *(int32_t*)(buf + 24);
    int32_t f4 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    return 0;
}
