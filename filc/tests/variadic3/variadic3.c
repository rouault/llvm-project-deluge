#include <stdarg.h>
#include <stdfil.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void test1(int x, ...)
{
    va_list args;
    va_start(args, x);
    ZASSERT(x == 666);
    ZASSERT(va_arg(args, char) == 'h');
    ZASSERT(va_arg(args, char) == 'e');
    ZASSERT(va_arg(args, char) == 'l');
    ZASSERT(va_arg(args, char) == 'l');
    ZASSERT(va_arg(args, char) == 'o');
}

static void test2(int64_t x, ...)
{
    va_list args;
    va_start(args, x);
    ZASSERT(x == 666);
    ZASSERT((char)va_arg(args, uint64_t) == 'h');
    ZASSERT((char)va_arg(args, uint64_t) == 'e');
    ZASSERT((char)va_arg(args, uint64_t) == 'l');
    ZASSERT((char)va_arg(args, uint64_t) == 'l');
    ZASSERT((char)va_arg(args, uint64_t) == 'o');
}

#define CMPFLOAT(x, y) do { \
        double my_x = (x); \
        double my_y = (y); \
        if (my_x == my_y) \
            break; \
        fprintf(stderr, "%s:%d: %s: assertion %s == %s failed.\n", \
                __FILE__, __LINE__, __PRETTY_FUNCTION__, #x, #y); \
        fprintf(stderr, "    %s = %lf.\n", #x, my_x); \
        fprintf(stderr, "    %s = %lf.\n", #y, my_y); \
        zerror("assertion failed"); \
    } while (false)

static void test3(char* x, ...)
{
    va_list args;
    va_start(args, x);
    ZASSERT(!strcmp(x, "hello"));
    CMPFLOAT(va_arg(args, float), 1.5);
    CMPFLOAT(va_arg(args, float), 5.25);
    CMPFLOAT(va_arg(args, float), 4.75);
}

static void test4(char* x, ...)
{
    va_list args;
    va_start(args, x);
    ZASSERT(!strcmp(x, "hello"));
    CMPFLOAT(va_arg(args, double), 1.5);
    CMPFLOAT(va_arg(args, double), 5.25);
    CMPFLOAT(va_arg(args, double), 4.75);
}

int main()
{
    test1(666, 'h', 'e', 'l', 'l', 'o');
    test2(666, 'h', 'e', 'l', 'l', 'o');
    test1(666, (uint64_t)'h', (uint64_t)'e', (uint64_t)'l', (uint64_t)'l', (uint64_t)'o');
    test2(666, (uint64_t)'h', (uint64_t)'e', (uint64_t)'l', (uint64_t)'l', (uint64_t)'o');
    test1(666, (unsigned)'h', (unsigned)'e', (unsigned)'l', (unsigned)'l', (unsigned)'o');
    test2(666, (unsigned)'h', (unsigned)'e', (unsigned)'l', (unsigned)'l', (unsigned)'o');
    test3("hello", (double)1.5, (double)5.25, (double)4.75);
    test3("hello", (float)1.5, (float)5.25, (float)4.75);
    test4("hello", (double)1.5, (double)5.25, (double)4.75);
    test4("hello", (float)1.5, (float)5.25, (float)4.75);
    return 0;
}

