#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(16));
    *(char**)(buf + 0) = "hello";
    buf = (char*)(buf) + -26208;
    char* f0 = *(char**)(buf + 0);
    ZASSERT(!strcmp(f0, "hello"));
    return 0;
}
