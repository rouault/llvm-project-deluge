#include <stdfil.h>

void foo(void) __attribute__((constructor));
void foo(void)
{
    zprintf("Nic o");
}

void bar(void) __attribute__((destructor));
void bar(void)
{
    zprintf("bez nas.\n");
}

int main(int argc, char** argv)
{
    zprintf(" nas ");
}

