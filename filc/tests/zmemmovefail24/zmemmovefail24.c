#include <stdfil.h>

int main()
{
    char* dst[4];
    __int128 src[4];

    unsigned i = 0;
    for (i = 4; i--;)
        dst[i] = "hello";

    zmemmove((char*)dst + 5, src, 1);

    return 0;
}
