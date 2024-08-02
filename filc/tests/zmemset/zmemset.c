#include <stdfil.h>

int main(void)
{
    char buf[10];
    zmemset(buf, 'f', 10);
    zmemset(buf + 9, 0, 1);
    zmemset((void*)666, 0, 0);
    zmemset((void*)666, 42, 0);
    zprint(buf);
    return 0;
}

