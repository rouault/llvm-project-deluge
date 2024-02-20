#include <stdfil.h>

int main()
{
    zrealloc(zalloc(char, 64) + 1, char, 128);
    return 0;
}


