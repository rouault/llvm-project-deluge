#include <stdio.h>
#include <stdfil.h>

static int foo(void)
{
    char buf[100];
    zreturn(buf, sizeof(buf));
}

int main()
{
    printf("%d\n", foo());
    return 0;
}

