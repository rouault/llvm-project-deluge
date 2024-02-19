#include <stdfil.h>

int main(void)
{
    int* ptr = zcalloc(int, 2, 24);
    ptr[0] = 666;
    if (ptr[0] == 666)
        zprint("YAY\n");
    ptr[100] = 1000;
    if (ptr[100] == 666)
        zprint("NO\n");
    return 0;
}


