#include <stdfil.h>

int main()
{
    char dst[64];
    char src[64];

    zmemmove(dst + 0x8000000000000000, src, 0x8000000000000000);

    return 0;
}

