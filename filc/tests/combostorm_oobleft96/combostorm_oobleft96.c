#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + -26208;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
    return 0;
}
