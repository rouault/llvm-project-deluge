#include <stdfil.h>
#include "utils.h"

int main(int argc, char** argv)
{
    int x = (int)opaque((void*)42);

    int array[x * 2];

    for (int i = 0; i < x * 2; ++i)
        ((int*)opaque(array))[i] = 666 + i;

    int result = 0;
    for (int i = 0; i < x * 2; ++i)
        result += array[i];

    zprintf("result = %d\n", result);

    return 0;
}

