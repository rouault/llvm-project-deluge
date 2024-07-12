#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    __int128* ptr = (__int128*)opaque(NULL);
    *ptr = 666;
    return 0;
}
