#include <stdfil.h>

int main()
{
    char* a = zgc_alloc(sizeof(char));
    char* b = zgc_alloc(sizeof(char));
    zgc_realloc(a + (b - a), 0);
    zprintf("oops\n");
    return 0;
}

