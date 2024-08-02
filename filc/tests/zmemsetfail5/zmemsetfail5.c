#include <stdfil.h>

int main()
{
    char buf[64];
    zmemset(buf - 100000000000, 42, 1);
    return 0;
}

