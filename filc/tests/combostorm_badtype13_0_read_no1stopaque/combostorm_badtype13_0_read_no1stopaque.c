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
    char* f0 = *(char**)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 22);
    int16_t f2 = *(int16_t*)(buf + 30);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    return 0;
}
