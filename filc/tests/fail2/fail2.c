#include <stdfil.h>

int* opaque(int*);

int main(void)
{
    int x[100];
    int* ptr = (int*)((char*)opaque(&x) + 99 * 4 + 2);
    *ptr = 42;
    if (*ptr == 42)
        zprint("YAY\n");
    else
        zerror("bad");
    return 0;
}

