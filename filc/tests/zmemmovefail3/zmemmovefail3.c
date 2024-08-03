#include <stdfil.h>

int main()
{
    char dst[64];
    char src[64];

    zmemmove(dst - 1, src, 64);

    return 0;
}

