#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(32));
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 26208;
    char* f0 = *(char**)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
    return 0;
}
