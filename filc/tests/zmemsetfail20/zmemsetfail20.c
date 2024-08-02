#include <stdfil.h>

int main()
{
    char* array[4];
    zmemset(array, 0, 1);
    unsigned i;
    for (i = 4; i--;)
        array[i] = "bruh";
    return 0;
}

