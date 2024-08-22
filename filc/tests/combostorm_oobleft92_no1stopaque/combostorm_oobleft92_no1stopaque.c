#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + -26208;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    return 0;
}
