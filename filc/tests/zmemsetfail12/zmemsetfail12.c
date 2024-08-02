#include <stdfil.h>
#include <inttypes.h>

int main()
{
    char buf[64];
    zmemset(buf, 42, (intptr_t)-1);
    return 0;
}

