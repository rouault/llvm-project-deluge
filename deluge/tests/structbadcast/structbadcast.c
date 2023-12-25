#include <stdfil.h>

void print(const char* string);
void print_long(long value);

struct foo {
    int x;
    char* string;
};

struct bar {
    char* string;
    int x;
};

#define PRINT(exp) do { \
        print(#exp " = "); \
        print_long(exp); \
        print("\n"); \
    } while (0)

int main(void) {
    PRINT(sizeof(struct foo));
    struct foo* ptr = zalloc(struct foo, 1);
    ptr->x = 42;
    ptr->string = "hello";
    struct bar* ptr2 = (struct bar*)ptr;
    if (ptr2->x == 42)
        print("NO\n");
    if (ptr2->string == "hello")
        print("NO\n");
    return 0;
}

