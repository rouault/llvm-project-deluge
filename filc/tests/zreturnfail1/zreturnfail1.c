#include <stdio.h>
#include <stdfil.h>

static int foo(void)
{
    int rets;
    zreturn(&rets + 10, sizeof(int));
}

int main()
{
    printf("%d\n", foo());
    return 0;
}

