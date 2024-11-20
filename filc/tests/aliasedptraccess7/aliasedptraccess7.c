#include <stdfil.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"

int main(int argc, char** argv)
{
    char* p;
    char** q = (char**)opaque(&p);
    char** r = (char**)opaque(&p);
    bool e = !opaque(NULL);
    char* a = *q;
    char* b = *q;
    if (e)
        *r = "hello";
    char* c = *q;
    ZASSERT(!a);
    ZASSERT(!b);
    ZASSERT(!strcmp(c, "hello"));
    return 0;
}


