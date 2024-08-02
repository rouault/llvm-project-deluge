#include <stdfil.h>

int main()
{
    char buf[64];
    zmemset(buf - 1, 42, 1);
    return 0;
}

