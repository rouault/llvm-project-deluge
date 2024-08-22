#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
    return 0;
}
