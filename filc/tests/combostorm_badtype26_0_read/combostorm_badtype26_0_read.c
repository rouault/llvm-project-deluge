#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(20));
    *(int32_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
    return 0;
}
