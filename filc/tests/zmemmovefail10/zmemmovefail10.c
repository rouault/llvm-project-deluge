#include <stdfil.h>

int main()
{
    char dst[64];
    char src[32];

    zmemmove(dst, src, 33);

    return 0;
}

