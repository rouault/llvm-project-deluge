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
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int16_t*)(buf + 6) == 42);
    ZASSERT(*(int16_t*)(buf + 14) == 42);
    ZASSERT(*(int16_t*)(buf + 22) == 42);
    ZASSERT(*(int16_t*)(buf + 30) == 42);
    return 0;
}
