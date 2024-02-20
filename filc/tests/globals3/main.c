#include <stdfil.h>
#include "header.h"

int main(int argc, char** argv)
{
    foo.y = 200;
    __builtin_memset(buf, 0, 666);
    __builtin_memset(buf, 'f', 665);
    zprintf("foo.x = %s\n", foo.x);
    zprintf("foo.y = %d\n", foo.y);
    return 0;
}

