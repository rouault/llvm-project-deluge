#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    ZASSERT(!strcmp(f3, "hello"));
    return 0;
}
