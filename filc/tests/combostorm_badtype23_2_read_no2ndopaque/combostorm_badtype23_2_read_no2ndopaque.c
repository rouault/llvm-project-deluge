#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    return 0;
}
