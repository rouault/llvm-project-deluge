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
    buf = (char*)opaque(buf) + -26208;
    ZASSERT(*(int32_t*)(buf + 4) == 42);
    ZASSERT(*(int32_t*)(buf + 12) == 42);
    ZASSERT(*(int32_t*)(buf + 20) == 42);
    ZASSERT(*(int32_t*)(buf + 28) == 42);
    return 0;
}
