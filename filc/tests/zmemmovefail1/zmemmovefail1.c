#include <stdfil.h>

int main()
{
    char src[64];

    zmemmove((void*)666, src, 64);

    return 0;
}

