#include <stdfil.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int thingy(char* x)
{
    int result;
    ZASSERT(sscanf(x, "%d", &result) == 1);
    return result;
}

int main()
{
    char* thingy_arg = "666";
    zcall(thingy, thingy_arg);
    return 0;
}
