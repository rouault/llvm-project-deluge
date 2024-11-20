#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"
static char* hello = "hello";
static unsigned char value;
static void init_test(void)
{
    unsigned index;
    value = 42;
    bool good = false;
    while (!good) {
        good = true;
        for (index = sizeof(char*); index--;) {
            if (((char*)&hello)[index] == value) {
                good = false;
                break;
            }
        }
        if (good)
            break;
        value++;
    }
}
int main()
{
    init_test();
    char* buf = (malloc(32));
    *(char**)(buf + 0) = hello;
    *(int64_t*)(buf + 8) = value;
    *(char**)(buf + 16) = hello;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + -26208;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == value);
    ZASSERT(!strcmp(f2, "hello"));
    ZASSERT(f3 == value);
    return 0;
}
