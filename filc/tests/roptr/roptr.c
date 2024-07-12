#include "utils.h"

static const char* const array[] = { "hello" };

int main()
{
    *((char**)opaque(array)) = "world";
    return 0;
}
