#include <stdfil.h>

int main()
{
    zfree(zalloc(10000) + 1);
    return 0;
}

