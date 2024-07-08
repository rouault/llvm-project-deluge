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

float* call_bar(void* the_bar, double value)
{
    zprint("in call_bar!\n");
    zprint("the_bar = ");
    zprint_ptr(the_bar);
    zprint("\n");
    float* result = ((float*(*)(double))opaque(the_bar))(value);
    zprint("in call_bar, result = ");
    zprint_ptr(result);
    zprint("\n");
    return result;
}

int main(void)
{
    zprint("starting!\n");
    if (call_foo(foo, "thingy\n") == 32)
        zprint("stuff\n");
    float* float_ptr = call_bar(bar, 42.);
    zprint("float_ptr = ");
    zprint_ptr(float_ptr);
    zprint("\n");
    if (*float_ptr == (float)42.)
        zprint("yay\n");
    zprint("here\n");
    call_foo(bar, "hello");
    zprint("nonono\n");
    return 0;
}


