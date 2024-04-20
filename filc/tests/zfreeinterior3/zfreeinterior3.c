#include <stdfil.h>

int main()
{
    zfree(zalloc(200000) + 1);
    return 0;
}

