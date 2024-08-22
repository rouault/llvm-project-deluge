#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(30));
    *(int8_t*)(buf + 5) = 42;
    *(int8_t*)(buf + 11) = 42;
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)(buf) + 26208;
    int8_t f0 = *(int8_t*)(buf + 5);
    int8_t f1 = *(int8_t*)(buf + 11);
    int8_t f2 = *(int8_t*)(buf + 17);
    int8_t f3 = *(int8_t*)(buf + 23);
    int8_t f4 = *(int8_t*)(buf + 29);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    return 0;
}
