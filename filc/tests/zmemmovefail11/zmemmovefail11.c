#include <stdfil.h>

int main()
{
    char dst[32];
    char src[64];

    zmemmove(dst, src, 33);

    return 0;
}

