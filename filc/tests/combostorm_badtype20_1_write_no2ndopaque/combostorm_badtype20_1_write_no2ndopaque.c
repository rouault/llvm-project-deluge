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
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 4);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 20);
    int32_t f3 = *(int32_t*)(buf + 28);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    return 0;
}
