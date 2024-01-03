#include <stdfil.h>

int main(int argc, char** argv)
{
    int foo;
    zprintf("Yo!\n");
    __builtin_memcpy(&foo, 0, sizeof(foo));
    zprintf("No!\n");
    return 0;
}


