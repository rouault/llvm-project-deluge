#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    long* ptr = (long*)opaque(NULL);
    opaque((void*)*ptr);
    return 0;
}
