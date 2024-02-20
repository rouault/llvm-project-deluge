#include <stdfil.h>

int main(void)
{
    zprintf("Yesyes\n");
    asm volatile("" : : : "memory");
    zprintf("Nono\n");
    return 0;
}

