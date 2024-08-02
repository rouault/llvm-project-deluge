#include <stdfil.h>

int main()
{
    char* array[4];
    zmemset((char*)array + 1, 0, 14);
    unsigned i;
    for (i = 4; i--;)
        array[i] = "bruh";
    return 0;
}

