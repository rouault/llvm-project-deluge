#include <stdfil.h>

int main()
{
    zgc_free(zgc_alloc(20000000) + 1);
    return 0;
}

