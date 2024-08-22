#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int16_t*)(buf + 18) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 26) = 42;
    *(int16_t*)(buf + 30) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 2);
    int16_t f1 = *(int16_t*)(buf + 6);
    int16_t f2 = *(int16_t*)(buf + 10);
    int16_t f3 = *(int16_t*)(buf + 14);
    int16_t f4 = *(int16_t*)(buf + 18);
    int16_t f5 = *(int16_t*)(buf + 22);
    int16_t f6 = *(int16_t*)(buf + 26);
    int16_t f7 = *(int16_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    return 0;
}
