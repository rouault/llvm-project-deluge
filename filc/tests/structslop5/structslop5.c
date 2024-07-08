#include <stdfil.h>

typedef struct Status Status;
struct Status {
    int a;
    char* b;
    char* c;
    int d;
};

__attribute__((__noinline__)) static Status foo(void)
{
    Status status;
    status.a = 1;
    status.b = "blah";
    status.c = "bleh";
    status.d = 2;
    return status;
}

__attribute__((__noinline__)) static Status bar(void)
{
    Status status;
    status.a = 42;
    return status;
}

__attribute__((__noinline__)) static void baz(Status s)
{
    zprintf("(3) %d\n", s.a);
}

int main()
{
    Status status;
    status = bar();
    zprintf("(1) %d\n", status.a);
    status = foo();
    zprintf("(2) %d\n", status.a);
    baz(status);
    return 0;
}

