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
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 17);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    return 0;
}
