#include <stdfil.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    char buf1[256];
    char buf2[256];
    size_t index;

    memset(buf1, 42, 256);
    for (index = 256; index--;)
        ZASSERT(buf1[index] == 42);

    memset(buf1 + 7, 0, 242);
    for (index = 7; index--;)
        ZASSERT(buf1[index] == 42);
    for (index = 249; index-->7;)
        ZASSERT(!buf1[index]);
    for (index = 256; index-->249;)
        ZASSERT(buf1[index] == 42);

    memset(buf2 + 7, 0, 242);
    int** ptr = (int**)buf2;
    for (index = 15; index-->1;) {
        ptr[index] = malloc(sizeof(int));
        *ptr[index] = (int)index;
    }
    for (index = 15; index-->1;)
        ZASSERT(*ptr[index] == (int)index);

    return 0;
}

