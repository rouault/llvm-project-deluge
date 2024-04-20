#include <stdfil.h>

struct foo {
    int x;
    char* string;
};

int main(void) {
    struct foo* ptr = zalloc(sizeof(struct foo));
    ptr->x = 42;
    ptr->string = "hello";
    if (ptr->x == 42)
        zprint("YAY1\n");
    if (ptr->string == "hello")
        zprint("YAY2\n");
    return 0;
}

