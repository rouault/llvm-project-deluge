#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
    return 0;
}
