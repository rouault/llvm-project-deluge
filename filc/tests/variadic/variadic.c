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
    zprint("yes = ");
    int result = foo(3, 666, 42, 1980);
    zprint_long(result);
    zprint("\n");
    return 0;
}

