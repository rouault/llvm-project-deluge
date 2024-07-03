#include <stdfil.h>
#include <stdlib.h>

static void test(size_t count, size_t size)
{
    while (count--) {
        char* p = zgc_alloc(size);
        ZASSERT(p);
        ZASSERT(!*p);
        *p = 42;
        zgc_free(p);
    }
}

static void test_ptr(size_t count, size_t size)
{
    while (count--) {
        char** p = zgc_alloc(sizeof(char*));
        ZASSERT(p);
        ZASSERT(!*p);
        *p = "hello";
        zgc_free(p);
    }
}

static void test_aligned(size_t count, size_t size, size_t alignment)
{
    while (count--) {
        char* p = zgc_aligned_alloc(alignment, size);
        ZASSERT(p);
        ZASSERT(!*p);
        *p = 42;
        zgc_free(p);
    }
}

struct foo {
    int a;
    int b;
    char c[];
};

static void test_int_flex(size_t count, size_t length)
{
    while (count--) {
        struct foo* p = zgc_alloc(__builtin_offsetof(struct foo, c) + length);
        ZASSERT(p);
        ZASSERT(!p->a);
        p->a = 42;
        zgc_free(p);
    }
}

struct bar {
    int a;
    int b;
    char* c[];
};

static void test_ptr_flex(size_t count, size_t length)
{
    while (count--) {
        struct bar* p = zgc_alloc(__builtin_offsetof(struct bar, c) + length * sizeof(char*));
        ZASSERT(p);
        ZASSERT(!p->a);
        p->a = 42;
        zgc_free(p);
    }
}

int main()
{
    zscavenger_suspend();

    test(10000, 16);
    test(10, 10000);
    test_ptr(10000, 1);
    test_ptr(10000, 2);
    test_aligned(1000, 64, 64);
    test_int_flex(10000, 1);
    test_ptr_flex(10000, 1);
    return 0;
}

