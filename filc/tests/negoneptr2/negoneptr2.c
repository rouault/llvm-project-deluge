#include <stdfil.h>

int main(int argc, char** argv)
{
    void* ptr = (void*)-1;
    zprint("ptr = ");
    zprint_ptr(ptr);
    zprint("\n");
    if (ptr == (void*)-1)
        zprint("Yay!\n");
    return 0;
}

