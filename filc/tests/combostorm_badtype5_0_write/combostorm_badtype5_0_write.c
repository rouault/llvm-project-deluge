#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(31));
    *(char**)(buf + 0) = "hello";
    *(int8_t*)(buf + 20) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 10);
    int8_t f2 = *(int8_t*)(buf + 20);
    int8_t f3 = *(int8_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    return 0;
}
