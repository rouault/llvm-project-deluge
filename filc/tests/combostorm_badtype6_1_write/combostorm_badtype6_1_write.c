#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(int8_t*)(buf + 9) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 9) == 42);
    ZASSERT(*(int8_t*)(buf + 19) == 42);
    ZASSERT(*(int8_t*)(buf + 29) == 42);
    return 0;
}
