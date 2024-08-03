#include <stdfil.h>

int main()
{
    zptrtable* src = zptrtable_new();
    __int128 dst;

    zmemmove(&dst, src, 16);

    return 0;
}

