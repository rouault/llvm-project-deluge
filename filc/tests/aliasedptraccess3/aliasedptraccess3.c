#include <stdfil.h>
#include <string.h>
#include "utils.h"

int main(int argc, char** argv)
{
    char* p;
    char** q = (char**)opaque(&p);
    char** r = (char**)opaque(&p);
    char* a = *q;
    *r = "hello";
    char* b = *q;
    ZASSERT(!a);
    ZASSERT(!strcmp(b, "hello"));
    return 0;
}

