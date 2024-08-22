#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    return 0;
}
