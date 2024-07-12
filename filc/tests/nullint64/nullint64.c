#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    long* ptr = (long*)opaque(NULL);
    *ptr = 666;
    return 0;
}
