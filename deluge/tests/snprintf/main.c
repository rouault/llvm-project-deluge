#include <stdfil.h>
#include "snprintf.h"

int main(void)
{
    char buf[100];
    rep_snprintf(buf, sizeof(buf), "x = %d, y = %lf, z = %s\n", 42, 66.6, "hello");
    zprint(buf);
    return 0;
}

