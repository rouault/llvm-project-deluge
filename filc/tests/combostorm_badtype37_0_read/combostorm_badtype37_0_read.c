#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    return 0;
}
