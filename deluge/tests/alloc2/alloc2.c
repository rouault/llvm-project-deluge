#include <stdfil.h>

void print(const char* string);

int main(void)
{
    int** ptr = zalloc(int*, 42);
    ptr[0] = zalloc(int, 1);
    ptr[0][0] = 666;
    if (ptr[0][0] == 666)
        print("YES\n");
    return 0;
}


