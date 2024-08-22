#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 12) == 42);
    ZASSERT(*(int32_t*)(buf + 28) == 42);
    return 0;
}
