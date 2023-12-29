#include <stdfil.h>

int foo(int x, ...)
{
    int result = 0;
    __builtin_va_list list;
    __builtin_va_start(list, x);
    while (x--) {
        int value = __builtin_va_arg(list, int);
        result += value;
    }
    __builtin_va_end(list);
    return result;
}

int main(void)
{
    foo(4, 666, 42, 1980);
    zprint("nono\n");
    return 0;
}

