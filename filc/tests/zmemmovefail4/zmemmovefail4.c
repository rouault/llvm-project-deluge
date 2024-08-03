#include <stdfil.h>

int main()
{
    char dst[64];
    char src[64];

    zmemmove(dst, src - 1, 64);

    return 0;
}

