#include <stdfil.h>

int main(int argc, char** argv)
{
    int foo;
    zprintf("Yo!\n");
    __builtin_memcpy(0, &foo, sizeof(foo));
    zprintf("No!\n");
    return 0;
}


