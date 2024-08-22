#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(28));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 24) = 42;
    buf = (char*)(buf) + -26208;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    return 0;
}
