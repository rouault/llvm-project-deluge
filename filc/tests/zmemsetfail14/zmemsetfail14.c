#include <stdfil.h>

int main()
{
    char* array[4];
    zmemset(array, 42, sizeof(array));
    unsigned i;
    zprintf("got this far\n");
    for (i = 4; i--;)
        array[i] = "hello";
    return 0;
}

