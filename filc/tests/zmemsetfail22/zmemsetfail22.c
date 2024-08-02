#include <stdfil.h>

int main()
{
    char* array[4];
    zmemset((char*)array + 15, 0, 1);
    unsigned i;
    for (i = 4; i--;)
        array[i] = "bruh";
    return 0;
}

