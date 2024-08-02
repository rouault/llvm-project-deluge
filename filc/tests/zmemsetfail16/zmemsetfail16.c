#include <stdfil.h>

int main()
{
    char* array[4];
    unsigned i;
    for (i = 4; i--;)
        array[i] = "hello";
    zprintf("got this far\n");
    zmemset((char*)array + 1, 42, sizeof(array) - 1);
    return 0;
}

