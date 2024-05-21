#include <string.h>

int main()
{
    memcmp("hello" + 100, "world", 5);
    return 0;
}

