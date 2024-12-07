#include <stdfil.h>
#include "header.h"

using namespace std;

void doit(Bar* bar)
{
    if (dynamic_cast<OtherBaz*>(bar))
        zprintf("tak\n");
    else
        zprintf("nie\n");
}
