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
    zprintf("(3) %s\n", s.c);
}

static inline Status call_foo(void)
{
    return foo();
}

static inline Status call_bar(void)
{
    return bar();
}

static inline void call_baz(Status s)
{
    baz(s);
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
    zprintf("(1) %d\n", status.a);
    status = call_foo();
    zprintf("(2) %d\n", status.a);
    call_baz(status);
    void (*func)(void);
    set_func(&func);
    func();
    return 0;
}

