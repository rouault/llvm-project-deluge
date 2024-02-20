#include <stdfil.h>

int main()
{
    char* a = zalloc(char, 1);
    char* b = zalloc(char, 1);
    zrealloc(a + (b - a), char, 0);
    zprintf("oops\n");
    return 0;
}

