#include <stdfil.h>

int main(int argc, char** argv)
{
    __builtin_memcpy(0, 0, 0);
    __builtin_memmove(0, 0, 0);
    __builtin_memset(0, '6', 0);
    zprintf("Yo!\n");
    return 0;
}

