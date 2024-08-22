#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
    return 0;
}
