#include <stdfil.h>

int main()
{
    char* a = zalloc(char, 1);
    char* b = zalloc(char, 1);
    zfree(a + (b - a));
    zprintf("oops\n");
    return 0;
}

