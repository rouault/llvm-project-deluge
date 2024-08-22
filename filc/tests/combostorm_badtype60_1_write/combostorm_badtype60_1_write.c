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
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
    return 0;
}
