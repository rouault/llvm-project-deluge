#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    int* ptr = (int*)opaque(NULL);
    *ptr = 666;
    return 0;
}
