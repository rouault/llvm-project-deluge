#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    int* ptr = (int*)opaque(NULL);
    opaque((void*)*ptr);
    return 0;
}
