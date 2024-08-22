#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(16));
    *(int64_t*)(buf + 8) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    ZASSERT(!strcmp(f0, "hello"));
    return 0;
}
