#include <stdfil.h>

int main()
{
    __int128 dst[4];
    char* src[4];

    unsigned i = 0;
    for (i = 4; i--;) {
        dst[i] = 666;
        src[i] = "hello";
    }

    zmemmove((char*)dst + 5, (char*)src + 5, 1);

    return 0;
}
