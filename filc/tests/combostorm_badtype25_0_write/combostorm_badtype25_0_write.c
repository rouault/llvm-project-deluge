#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(24));
    *(char**)(buf + 0) = "hello";
    *(int32_t*)(buf + 20) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 0) == 42);
    ZASSERT(*(int32_t*)(buf + 20) == 42);
    return 0;
}
