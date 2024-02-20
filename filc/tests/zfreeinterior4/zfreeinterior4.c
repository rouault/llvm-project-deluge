#include <stdfil.h>

int main()
{
    zfree(zrestrict(zalloc(char, 20000000) + 1, char, 1));
    return 0;
}

