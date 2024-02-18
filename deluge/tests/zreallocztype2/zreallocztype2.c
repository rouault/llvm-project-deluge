#include <stdfil.h>

int main(int argc, char** argv)
{
    ztype* type = zgettype(argv);
    zprintf("Spoko\n");
    zprintf("type = %p\n", type);
    zrealloc(type, char, 0);
    zprintf("Nie dobrze\n");
    return 0;
}

