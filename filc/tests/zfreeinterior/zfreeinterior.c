#include <stdfil.h>

int main()
{
    zgc_free(zgc_alloc(64) + 1);
    return 0;
}

