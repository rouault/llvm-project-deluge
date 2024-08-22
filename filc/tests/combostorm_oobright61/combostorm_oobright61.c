#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 26208;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
    return 0;
}
