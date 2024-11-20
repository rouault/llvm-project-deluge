#include <stdfil.h>
#include <string.h>
#include "utils.h"

int main(int argc, char** argv)
{
    char* p;
    char** q = (char**)opaque(&p);
    char** r = (char**)opaque(&p);
    *r = "hello";
    ZASSERT(!strcmp(*q, "hello"));
    return 0;
}

