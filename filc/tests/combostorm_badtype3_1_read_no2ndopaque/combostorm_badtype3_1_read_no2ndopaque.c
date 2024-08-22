#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 6) = 42;
    *(int8_t*)(buf + 12) = 42;
    *(int8_t*)(buf + 18) = 42;
    *(int8_t*)(buf + 24) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 6);
    int8_t f2 = *(int8_t*)(buf + 12);
    char* f3 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
    return 0;
}
