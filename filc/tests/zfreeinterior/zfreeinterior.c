#include <stdfil.h>

int main()
{
    zfree(zalloc(64) + 1);
    return 0;
}

