#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"
static char* hello = "hello";
static unsigned char value;
static void init_test(void)
{
    unsigned index;
    value = 42;
    bool good = false;
    while (!good) {
        good = true;
        for (index = sizeof(char*); index--;) {
            if (((char*)&hello)[index] == value) {
                good = false;
                break;
            }
        }
        if (good)
            break;
        value++;
    }
}
static void test0(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test1(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test2(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test3(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test4(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test5(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test6(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test7(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test8(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test9(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test10(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test11(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test12(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test13(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test14(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test15(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test16(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test17(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test18(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test19(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test20(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test21(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test22(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test23(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 16) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test24(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test25(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test26(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test27(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test28(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test29(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test30(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test31(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test32(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == value);
}
static void test33(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test34(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == value);
}
static void test35(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test36(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == value);
}
static void test37(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test38(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == value);
}
static void test39(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test40(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test41(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test42(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test43(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test44(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test45(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test46(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test47(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(char**)(buf + 16) = hello;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == value);
}
static void test48(void)
{
    char* buf = opaque(malloc(16));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test49(void)
{
    char* buf = opaque(malloc(16));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test50(void)
{
    char* buf = (malloc(16));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test51(void)
{
    char* buf = (malloc(16));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test52(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test53(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test54(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test55(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(int64_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test56(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test57(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test58(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test59(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test60(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test61(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test62(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test63(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = value;
    *(int64_t*)(buf + 8) = value;
    *(char**)(buf + 24) = hello;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
int main()
{
    init_test();
    test0();
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
    test11();
    test12();
    test13();
    test14();
    test15();
    test16();
    test17();
    test18();
    test19();
    test20();
    test21();
    test22();
    test23();
    test24();
    test25();
    test26();
    test27();
    test28();
    test29();
    test30();
    test31();
    test32();
    test33();
    test34();
    test35();
    test36();
    test37();
    test38();
    test39();
    test40();
    test41();
    test42();
    test43();
    test44();
    test45();
    test46();
    test47();
    test48();
    test49();
    test50();
    test51();
    test52();
    test53();
    test54();
    test55();
    test56();
    test57();
    test58();
    test59();
    test60();
    test61();
    test62();
    test63();
    return 0;
}
