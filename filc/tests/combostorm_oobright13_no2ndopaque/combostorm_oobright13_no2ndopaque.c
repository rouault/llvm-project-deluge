#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 30) = 42;
    buf = (char*)(buf) + 26208;
    int16_t f0 = *(int16_t*)(buf + 6);
    int16_t f1 = *(int16_t*)(buf + 14);
    int16_t f2 = *(int16_t*)(buf + 22);
    int16_t f3 = *(int16_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    return 0;
}
