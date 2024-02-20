#include <stdfil.h>

int main()
{
    zhard_free(zrestrict(zhard_alloc(char, 64) + 1, char, 1));
    return 0;
}

