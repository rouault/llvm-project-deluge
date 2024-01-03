#include <stdfil.h>
#include "header.h"

int main(int argc, char** argv)
{
    int x = foo("hello");
    int y = bar("world");
    zprintf("x = %d, y = %d\n", x, y);
    return 0;
}


