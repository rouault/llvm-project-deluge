#include <stdfil.h>

int main()
{
    zfree(zalloc(20000000) + 1);
    return 0;
}

