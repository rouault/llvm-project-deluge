#include <stdfil.h>

int main()
{
    char* src[2];
    src[0] = "hello";
    src[1] = "world";
    __int128 dst = 666;

    zmemmove(&dst, (char*)src + 1, 16);

    return 0;
}
