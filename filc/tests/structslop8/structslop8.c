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

__attribute__((__noinline__)) static void baz(int x, Status s)
{
    zprintf("(%d) %s\n", x, s.c);
}

static inline Status call_foo(void)
{
    return foo();
}

static inline Status call_bar(void)
{
    return bar();
}

static inline void call_baz(int x, Status s)
{
    baz(x, s);
}

static void the_func(void)
{
    zprintf("the func!\n");
}

static inline void set_func(void (**func)(void))
{
    *func = the_func;
}

int main()
{
    Status status;
    status = call_bar();
    if (status.a)
        call_baz(1, status);
    status = call_foo();
    if (status.a)
        call_baz(2, status);
    void (*func)(void);
    set_func(&func);
    func();
    return 0;
}

