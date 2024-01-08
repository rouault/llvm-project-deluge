#include <stdfil.h>

void foo(void) __attribute__((constructor));
void foo(void)
{
    zprintf("Hello ");
}

int main(int argc, char** argv)
{
    zprintf("World!\n");
}

