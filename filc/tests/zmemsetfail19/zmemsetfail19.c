#include <stdfil.h>

int main()
{
    char* array[4];
    unsigned i;
    for (i = 4; i--;)
        array[i] = "bruh";
    zmemset(array, 0, 1);
    return 0;
}

