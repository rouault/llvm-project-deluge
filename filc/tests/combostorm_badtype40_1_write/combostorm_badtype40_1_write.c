#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
    return 0;
}
