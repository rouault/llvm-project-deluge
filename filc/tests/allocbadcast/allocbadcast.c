#include <stdfil.h>

int main(void)
{
    int** ptr = (int**)zalloc(int, 42);
    ptr[0] = zalloc(int, 1);
    ptr[0][0] = 666;
    if (ptr[0][0] == 666)
        zprint("NO\n");
    return 0;
}


