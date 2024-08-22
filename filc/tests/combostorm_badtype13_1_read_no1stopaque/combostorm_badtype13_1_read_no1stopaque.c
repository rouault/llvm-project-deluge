#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(32));
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 6);
    int16_t f1 = *(int16_t*)(buf + 14);
    char* f2 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    return 0;
}
