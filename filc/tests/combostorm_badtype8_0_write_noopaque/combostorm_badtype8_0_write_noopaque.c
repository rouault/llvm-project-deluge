#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(32));
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 16);
    ZASSERT(f0 == 42);
    return 0;
}
