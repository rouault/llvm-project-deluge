#include <stdfil.h>

int main()
{
    zgc_free(zgc_alloc(10000) + 1);
    return 0;
}

