#include <stdfil.h>
#include <stdlib.h>

static void test(size_t count, size_t size)
{
    while (count--) {
        char* p = zhard_alloc(char, size);
        ZASSERT(p);
        ZASSERT(!*p);
        *p = 42;
        zhard_free(p);
        ZASSERT(!*p);
        *p = 42;
    }
}

static void test_ptr(size_t count, size_t size)
{
    while (count--) {
        char** p = zhard_alloc(char*, size);
        ZASSERT(p);
        ZASSERT(!*p);
        *p = "hello";
        zhard_free(p);
        ZASSERT(!*p);
        *p = "hello";
    }
}

static void test_aligned(size_t count, size_t size, size_t alignment)
{
    while (count--) {
        char* p = zhard_aligned_alloc(char, alignment, size);
        ZASSERT(p);
        ZASSERT(!*p);
        *p = 42;
        zhard_free(p);
        ZASSERT(!*p);
        *p = 42;
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
        struct foo* p = zhard_alloc_flex(struct foo, c, length);
        ZASSERT(p);
        ZASSERT(!p->a);
        p->a = 42;
        zhard_free(p);
        ZASSERT(!p->a);
        p->a = 42;
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
        struct bar* p = zhard_alloc_flex(struct bar, c, length);
        ZASSERT(p);
        ZASSERT(!p->a);
        p->a = 42;
        zhard_free(p);
        ZASSERT(!p->a);
        p->a = 42;
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

