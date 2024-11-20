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
    *(int64_t*)(buf + 16) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + -26208;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    char* f3 = *(char**)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(!strcmp(f3, "hello"));
    return 0;
}
