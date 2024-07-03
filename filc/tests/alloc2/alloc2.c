#include <stdfil.h>

int main(void)
{
    int** ptr = zgc_alloc(sizeof(int*) * 42);
    ptr[0] = zgc_alloc(sizeof(int));
    ptr[0][0] = 666;
    if (ptr[0][0] == 666)
        zprint("YES\n");
    return 0;
}


