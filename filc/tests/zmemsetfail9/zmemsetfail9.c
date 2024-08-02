#include <stdfil.h>
#include <inttypes.h>

int main()
{
    char buf[64];
    zmemset(buf, 0, (intptr_t)-1);
    return 0;
}

