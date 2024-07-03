#include <stdfil.h>

int main(int argc, char** argv)
{
    int* huge = zgc_alloc(sizeof(int) * 0x4000000000000000lu);
    zprintf("huge = %p\n", huge);
    *huge = 42;
    zprintf("*huge = %d\n", *huge);
    return 0;
}

