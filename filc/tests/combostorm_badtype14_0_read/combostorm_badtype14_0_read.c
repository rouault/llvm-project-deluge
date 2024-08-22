#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(26));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 12) = 42;
    *(int16_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    return 0;
}
