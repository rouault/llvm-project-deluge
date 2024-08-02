#include <stdfil.h>

int main()
{
    char* array[4];
    zmemset((char*)array + 1, 42, sizeof(array) - 1);
    unsigned i;
    zprintf("got this far\n");
    for (i = 4; i--;)
        array[i] = "hello";
    return 0;
}

