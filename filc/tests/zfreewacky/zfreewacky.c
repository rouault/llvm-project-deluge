#include <stdfil.h>

int main()
{
    char* a = zalloc(1);
    char* b = zalloc(1);
    zfree(a + (b - a));
    zprintf("oops\n");
    return 0;
}

