#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 6) = 42;
    *(int8_t*)(buf + 12) = 42;
    *(int8_t*)(buf + 18) = 42;
    *(int8_t*)(buf + 24) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 0) == 42);
    ZASSERT(*(int8_t*)(buf + 6) == 42);
    ZASSERT(*(int8_t*)(buf + 12) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    return 0;
}
