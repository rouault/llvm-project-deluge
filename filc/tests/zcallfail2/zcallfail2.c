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
    char* thingy_arg = "666";
    zcall_void(stuff, &thingy_arg);
    
    return 0;
}
