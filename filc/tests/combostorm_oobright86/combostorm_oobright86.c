#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 26208;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    return 0;
}
