#include <stdfil.h>
#include <inttypes.h>
#include <string.h>

int main()
{
    char* src[2];
    src[0] = "hello";
    src[1] = "world";
    int64_t dst = 666;

    zmemmove(&dst, (char*)src + 1, 8);

    ZASSERT(!zhasvalidcap(*(char**)&dst));

    return 0;
}
