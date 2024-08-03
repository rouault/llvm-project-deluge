#include <stdfil.h>

int main()
{
    char dst[64];
    char src[64];

    zmemmove(dst, src + 64, 64);

    return 0;
}

