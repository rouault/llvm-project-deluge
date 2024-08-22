#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    return 0;
}
