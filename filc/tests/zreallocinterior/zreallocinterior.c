#include <stdfil.h>

int main()
{
    zgc_realloc(zgc_alloc(64) + 1, 128);
    return 0;
}


