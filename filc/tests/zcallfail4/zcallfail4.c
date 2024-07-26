#include <stdfil.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char* stuff(int x)
{
    return zasprintf("%d", x);
}

int main()
{
    int stuff_arg = 666;
    zcall_int(stuff, &stuff_arg);
    
    return 0;
}
