#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(30));
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int8_t*)(buf + 19) == 42);
    ZASSERT(*(int8_t*)(buf + 29) == 42);
    return 0;
}
