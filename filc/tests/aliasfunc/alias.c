#include <stdfil.h>
#include "header.h"

int foo(const char* str)
{
    zprintf("str = %s\n", str);
    return 42;
}

int bar(const char* str) __attribute__((__weak__, __alias__("foo")));
