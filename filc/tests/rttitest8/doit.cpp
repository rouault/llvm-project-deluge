#include <stdfil.h>
#include "header.h"

using namespace std;

void doit(Bar* bar)
{
    if (OtherBaz* x = dynamic_cast<OtherBaz*>(bar))
        zprintf("tak");
    else
        zprintf("nie");
    zprintf("\n");
}
