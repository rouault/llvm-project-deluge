#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(48));
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    return 0;
}
