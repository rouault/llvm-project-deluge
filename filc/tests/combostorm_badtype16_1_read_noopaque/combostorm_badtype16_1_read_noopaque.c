#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(20));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 18) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    return 0;
}
