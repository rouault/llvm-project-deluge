#include <stdfil.h>

int main()
{
    char* dst = "hello";
    __int128 src[2];
    src[0] = 666;
    src[1] = 1410;

    zmemmove(&dst, (char*)src + 1, 16);

    return 0;
}
