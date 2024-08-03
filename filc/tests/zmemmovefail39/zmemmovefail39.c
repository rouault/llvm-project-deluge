#include <stdfil.h>

int main()
{
    zptrtable* dst = zptrtable_new();
    __int128 src;

    zmemmove(dst, &src, 16);

    return 0;
}

