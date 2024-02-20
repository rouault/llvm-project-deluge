#include <stdfil.h>
#include "utils.h"

int main(int argc, char** argv)
{
    int x;
    *(int*)opaque(&x) = 42;
    zprintf("Thing = %s\n", x == 42 ? "hello" : "world");
    return 0;
}

