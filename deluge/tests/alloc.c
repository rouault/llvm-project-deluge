#include <stdfil.h>

void print(const char* string);

int main(void)
{
    int* ptr = zalloc(int, 42);
    ptr[0] = 666;
    if (ptr[0] == 666)
        print("YAY\n");
    ptr[100] = 1000;
    if (ptr[100] == 666)
        print("NO\n");
    return 0;
}


