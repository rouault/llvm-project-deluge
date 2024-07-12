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
    int* ptr = (int*)opaque(zthread_create(thread_main, dude));
    opaque((void*)*ptr);
    return 0;
}
