#include <stdlib.h>
#include "utils.h"

int main()
{
    *((short*)opaque("nic o nas bez nas")) = 42;
    return 0;
}
