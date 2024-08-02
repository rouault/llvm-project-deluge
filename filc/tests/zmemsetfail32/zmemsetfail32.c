#include <stdfil.h>
#include <stdlib.h>
#include "utils.h"

int main()
{
    char* buf = malloc(16);
    free(opaque(buf));
    zmemset(buf, 42, 16);
    return 0;
}


