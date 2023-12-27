#include <stdfil.h>

int main(void)
{
    int x;
    x = 42;
    if (x == 42)
        zprint("YAY\n");
    else
        zerror("bad value");
    return 0;
}

