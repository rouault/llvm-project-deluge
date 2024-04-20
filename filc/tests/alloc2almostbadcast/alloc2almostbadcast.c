#include <stdfil.h>

int main(void)
{
    int** ptr = zalloc(sizeof(int*) * 42);
    ptr[0] = zalloc(sizeof(int));
    ptr[0][0] = 666;
    if (ptr[0][0] == 666)
        zprint("YES\n");
    float* ptr2 = (float*)ptr;
    ptr2[42] = 42.5;
    return 0;
}


