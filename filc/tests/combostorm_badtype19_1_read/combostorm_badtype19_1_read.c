#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 24) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 0) == 42);
    ZASSERT(*(int32_t*)(buf + 8) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(*(int32_t*)(buf + 32) == 42);
    return 0;
}
