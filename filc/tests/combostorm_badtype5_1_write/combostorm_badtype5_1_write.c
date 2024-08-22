#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 10) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 0) == 42);
    ZASSERT(*(int8_t*)(buf + 10) == 42);
    ZASSERT(*(int8_t*)(buf + 20) == 42);
    ZASSERT(*(int8_t*)(buf + 30) == 42);
    return 0;
}
