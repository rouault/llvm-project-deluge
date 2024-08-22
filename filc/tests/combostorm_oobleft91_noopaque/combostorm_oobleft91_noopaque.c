#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + -26208;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
    return 0;
}
