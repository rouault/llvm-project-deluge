#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(int32_t*)(buf + 4) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 20);
    int32_t f2 = *(int32_t*)(buf + 28);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    return 0;
}
