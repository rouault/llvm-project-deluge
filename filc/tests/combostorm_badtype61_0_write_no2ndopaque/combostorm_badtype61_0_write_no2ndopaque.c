#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
    return 0;
}
