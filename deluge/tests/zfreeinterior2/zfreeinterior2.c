#include <stdfil.h>

int main()
{
    zfree(zrestrict(zalloc(char, 10000) + 1, char, 1));
    return 0;
}

