#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(24));
    *(int16_t*)(buf + 10) = 42;
    *(int16_t*)(buf + 22) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int16_t*)(buf + 22) == 42);
    return 0;
}
