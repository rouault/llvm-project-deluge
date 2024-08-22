#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + -26208;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(*(int64_t*)(buf + 32) == 42);
    return 0;
}
