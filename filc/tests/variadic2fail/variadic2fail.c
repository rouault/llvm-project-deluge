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
    int* x = zalloc(int, 1);
    int* y = zalloc(int, 1);
    *x = 666;
    *y = 42;
    foo(3, x, y);
    zprint("nono\n");
    return 0;
}

