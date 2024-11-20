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
    char* buf = (malloc(25));
    *(int8_t*)(buf + 0) = value;
    *(int8_t*)(buf + 6) = value;
    *(int8_t*)(buf + 12) = value;
    *(int8_t*)(buf + 18) = value;
    *(int8_t*)(buf + 24) = value;
    buf = (char*)(buf) + -26208;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 6);
    int8_t f2 = *(int8_t*)(buf + 12);
    int8_t f3 = *(int8_t*)(buf + 18);
    int8_t f4 = *(int8_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    return 0;
}
