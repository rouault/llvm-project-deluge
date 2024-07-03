#include <stdfil.h>

int main()
{
    char* a = zgc_alloc(1);
    char* b = zgc_alloc(1);
    zgc_free(a + (b - a));
    zprintf("oops\n");
    return 0;
}

