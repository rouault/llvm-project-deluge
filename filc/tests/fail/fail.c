#include <stdfil.h>

int* opaque(int*);

int main(void)
{
    int x;
    int* ptr = opaque(&x) + 1;
    *ptr = 42;
    if (*ptr == 42)
        zprint("YAY\n");
    else
        zerror("fail");
    return 0;
}

