#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(int8_t*)(buf + 1) = 42;
    *(int8_t*)(buf + 3) = 42;
    *(int8_t*)(buf + 5) = 42;
    *(int8_t*)(buf + 7) = 42;
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 11) = 42;
    *(int8_t*)(buf + 13) = 42;
    *(int8_t*)(buf + 15) = 42;
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 21) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 25) = 42;
    *(int8_t*)(buf + 27) = 42;
    *(int8_t*)(buf + 29) = 42;
    *(int8_t*)(buf + 31) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 17);
    int8_t f2 = *(int8_t*)(buf + 19);
    int8_t f3 = *(int8_t*)(buf + 21);
    int8_t f4 = *(int8_t*)(buf + 23);
    int8_t f5 = *(int8_t*)(buf + 25);
    int8_t f6 = *(int8_t*)(buf + 27);
    int8_t f7 = *(int8_t*)(buf + 29);
    int8_t f8 = *(int8_t*)(buf + 31);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    return 0;
}
