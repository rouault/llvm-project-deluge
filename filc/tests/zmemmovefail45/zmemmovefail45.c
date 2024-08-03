#include <stdfil.h>

int main()
{
    char* dst = "hello";
    __int128 src = 666;

    zmemmove(&dst, &src, 16);

    return 0;
}
