#include <stdlib.h>
#include "utils.h"

int main()
{
    char* ptr = malloc(16);
    *((__int128*)opaque(ptr + 8)) = 42;
    return 0;
}
