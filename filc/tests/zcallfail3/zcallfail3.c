#include <stdfil.h>
#include <stdio.h>

static int thingy(void) { return 666; }

int main()
{
    char* x = zcall_value(thingy, NULL, char*);
    printf("%s\n", x);
    return 0;
}

