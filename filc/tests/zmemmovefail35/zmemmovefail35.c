#include <stdfil.h>

int main()
{
    __int128 dst[4];
    char* src[4];

    unsigned i = 0;
    for (i = 4; i--;)
        src[i] = "hello";

    zmemmove(dst, (char*)src + 5, 59);

    return 0;
}
