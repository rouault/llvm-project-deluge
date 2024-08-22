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
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    return 0;
}
