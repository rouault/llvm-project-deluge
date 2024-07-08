#include <stdfil.h>
#include "utils.h"

int foo(char* string)
{
    zprint(string);
    return 32;
}

float* bar(double value)
{
    float* x = (float*)zgc_alloc(sizeof(float));
    *x = (float)value;
    return x;
}

int call_foo(void* the_foo, char* string)
{
    return ((int(*)(char*))opaque(the_foo))(string);
}

int* call_bar_badly(void* the_bar, long long value)
{
    return ((int*(*)(long long))opaque(the_bar))(value);
}

int main(void)
{
    if (call_foo(foo, "thingy\n") == 32)
        zprint("stuff\n");
    int* int_ptr = call_bar_badly(bar, 4649074691427585229ll);
    zprint_long(*int_ptr);
    if (*int_ptr == 1143383654)
        zprint("yay\n");
    call_bar_badly(foo, 666);
    zprint("nonono\n");
    return 0;
}


