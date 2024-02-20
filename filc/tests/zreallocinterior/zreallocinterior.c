#include <stdfil.h>

int main()
{
    zrealloc(zrestrict(zalloc(char, 64) + 1, char, 1), char, 128);
    return 0;
}


