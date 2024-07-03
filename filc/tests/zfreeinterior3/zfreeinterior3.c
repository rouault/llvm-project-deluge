#include <stdfil.h>

int main()
{
    zgc_free(zgc_alloc(200000) + 1);
    return 0;
}

