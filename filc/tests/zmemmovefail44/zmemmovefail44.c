#include <stdfil.h>

int main()
{
    char* src = "hello";
    __int128 dst = 666;

    zmemmove(&dst, &src, 16);

    return 0;
}
