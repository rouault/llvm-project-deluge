#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    return 0;
}
