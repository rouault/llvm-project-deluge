#include <stdfil.h>

int main()
{
    zptrtable* dst = zptrtable_new();
    __int128 src = 666;

    zmemmove(dst, &src, 16);

    return 0;
}

