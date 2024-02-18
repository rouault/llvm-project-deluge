#include <stdfil.h>

int main()
{
    zfree(zrestrict(zalloc(char, 200000) + 1, char, 1));
    return 0;
}

