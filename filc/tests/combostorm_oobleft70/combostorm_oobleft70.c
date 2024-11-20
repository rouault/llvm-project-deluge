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
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 8) = hello;
    *(int64_t*)(buf + 16) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + -26208;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    char* f3 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == value);
    ZASSERT(!strcmp(f3, "hello"));
    return 0;
}
