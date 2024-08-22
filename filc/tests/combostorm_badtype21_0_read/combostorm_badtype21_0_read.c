#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(28));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int32_t*)(buf + 24) == 42);
    return 0;
}
