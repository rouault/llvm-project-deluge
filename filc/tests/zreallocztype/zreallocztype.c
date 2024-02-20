#include <stdfil.h>

int main(int argc, char** argv)
{
    ztype* type = zgettype(argv);
    zprintf("Spoko\n");
    zrealloc(type, char, 1);
    zprintf("Nie dobrze\n");
    return 0;
}

