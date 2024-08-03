#include <stdfil.h>

int main()
{
    char* dst[4];
    __int128 src[4];

    unsigned i = 0;
    for (i = 4; i--;) {
        dst[i] = "hello";
        src[i] = 666;
    }

    zmemmove(dst, (char*)src + 5, 1);

    return 0;
}
