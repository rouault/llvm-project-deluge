#include <stdfil.h>

int foo(int x, ...)
{
    int result = 0;
    __builtin_va_list list;
    __builtin_va_start(list, x);
    while (x--) {
        int value = *__builtin_va_arg(list, int*);
        result += value;
    }
    __builtin_va_end(list);
    return result;
}

int main(void)
{
    zprint("yes = ");
    int* x = zalloc(sizeof(int));
    int* y = zalloc(sizeof(int));
    int* z = zalloc(sizeof(int));
    *x = 666;
    *y = 42;
    *z = 1980;
    int result = foo(3, x, y, z);
    zprint_long(result);
    zprint("\n");
    return 0;
}

