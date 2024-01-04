#include <stdfil.h>
#include "utils.h"

int main(int argc, char** argv)
{
    int n;
    *((int*)opaque(&n)) = 42;
    int result = 0;
    for (int i = 0; i < n; ++i) {
        int* array = __builtin_alloca(sizeof(int) * i);
        for (int j = 0; j < i; ++j)
            ((int*)opaque(array))[j] = 666 * i + 42 * j;
        for (int j = 0; j < i; ++j)
            result += array[j];
    }
    zprintf("result = %d\n", result);
    return 0;
}

