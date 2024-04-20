#include <stdfil.h>

int main()
{
    char* a = zalloc(sizeof(char));
    char* b = zalloc(sizeof(char));
    zrealloc(a + (b - a), 0);
    zprintf("oops\n");
    return 0;
}

