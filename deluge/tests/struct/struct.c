#include <stdfil.h>

void print(const char* string);

struct foo {
    int x;
    char* string;
};

int main(void) {
    struct foo* ptr = zalloc(struct foo, 1);
    ptr->x = 42;
    ptr->string = "hello";
    if (ptr->x == 42)
        print("YAY1\n");
    if (ptr->string == "hello")
        print("YAY2\n");
    return 0;
}

