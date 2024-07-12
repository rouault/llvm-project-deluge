#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    char* ptr = (char*)opaque(NULL);
    opaque((void*)*ptr);
    return 0;
}
