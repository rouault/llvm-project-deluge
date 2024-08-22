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
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    return 0;
}
