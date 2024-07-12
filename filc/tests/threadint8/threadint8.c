#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>
#include <pizlonated_runtime.h>

static const char* dude = "dude";
    
static void* thread_main(void* arg)
{
    ZASSERT(arg == dude);
    return NULL;
}
    
int main()
{
    char* ptr = (char*)opaque(zthread_create(thread_main, dude));
    *ptr = 42;
    return 0;
}
