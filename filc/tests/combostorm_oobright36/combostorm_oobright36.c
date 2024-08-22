#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 26208;
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
    return 0;
}
