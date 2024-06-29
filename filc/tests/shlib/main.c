#include <stdfil.h>

int thingy(void);
int stuff(void);

int main()
{
    ZASSERT(thingy() == 666);
    ZASSERT(stuff() == 666);
    return 0;
}

