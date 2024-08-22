#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
    return 0;
}
