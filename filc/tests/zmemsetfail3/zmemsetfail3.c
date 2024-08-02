#include <stdfil.h>

int main()
{
    char buf[64];
    zmemset(buf - 1, 0, 16);
    return 0;
}

