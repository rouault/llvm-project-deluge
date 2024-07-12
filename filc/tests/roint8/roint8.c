#include <stdlib.h>
#include "utils.h"

int main()
{
    *((char*)opaque("nic o nas bez nas")) = 42;
    return 0;
}
