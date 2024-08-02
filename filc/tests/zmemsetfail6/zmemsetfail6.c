#include <stdfil.h>

int main()
{
    char buf[64];
    zmemset(buf + 64, 42, 1);
    return 0;
}

