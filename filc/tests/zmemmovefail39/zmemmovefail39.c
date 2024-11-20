#include <stdfil.h>
#include <inttypes.h>

int main()
{
    zptrtable* dst = zptrtable_new();
    int64_t src;

    zmemmove(dst, &src, 16);

    return 0;
}

