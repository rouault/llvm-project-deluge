#include <stdfil.h>
#include "utils.h"

void foo() { }
void bar() { }

int main(int argc, char** argv)
{
    int x;
    *(int*)opaque(&x) = 42;
    zprintf("Thing = %s\n", x == 42 ? (foo(), "hello") : (bar(), "world"));
    return 0;
}

