#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
    return 0;
}
