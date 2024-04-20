#include <stdfil.h>

int main()
{
    zrealloc(zalloc(64) + 1, 128);
    return 0;
}


