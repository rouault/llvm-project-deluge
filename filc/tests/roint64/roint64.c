#include <stdlib.h>
#include "utils.h"

int main()
{
    *((long*)opaque("nic o nas bez nas")) = 42;
    return 0;
}
