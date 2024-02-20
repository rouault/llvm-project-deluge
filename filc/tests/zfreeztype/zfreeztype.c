#include <stdfil.h>

int main(int argc, char** argv)
{
    ztype* type = zgettype(argv);
    zprintf("Spoko\n");
    zfree(type);
    zprintf("Nie dobrze\n");
    return 0;
}

