#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
    return 0;
}
