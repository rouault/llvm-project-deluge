#include <stdfil.h>

int main(int argc, char** argv)
{
    void* specific = zthread_key_create(0);
    zprintf("value before set: %p\n", zthread_getspecific(specific));
    zthread_setspecific(specific, (void*)666);
    zprintf("value after set: %p\n", zthread_getspecific(specific));
    zthread_key_delete(specific);
    zprintf("value after delete: %p\n", zthread_getspecific(specific));
    return 0;
}

