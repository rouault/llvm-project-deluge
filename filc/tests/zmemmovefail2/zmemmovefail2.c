#include <stdfil.h>

int main()
{
    char dst[64];

    zmemmove(dst, (void*)666, 64);

    return 0;
}

