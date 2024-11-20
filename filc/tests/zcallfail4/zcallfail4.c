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
    ZASSERT(!strcmp(*(char**)zcall(stuff, &stuff_arg), "666"));
    
    return 0;
}
