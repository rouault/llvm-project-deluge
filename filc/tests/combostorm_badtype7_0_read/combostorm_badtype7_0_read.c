#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(18));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 17) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int8_t*)(buf + 17) == 42);
    return 0;
}
