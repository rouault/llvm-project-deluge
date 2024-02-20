#include <stdfil.h>

int main(void)
{
    char buf[10];
    __builtin_memset(buf, 'f', 10);
    __builtin_memset(buf + 9, 0, 1);
    zprint(buf);
    return 0;
}

