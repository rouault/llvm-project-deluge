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
    char* buf = opaque(malloc(26));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 4) = value;
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 12) = value;
    *(int16_t*)(buf + 16) = value;
    *(int16_t*)(buf + 20) = value;
    *(int16_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 4);
    int16_t f2 = *(int16_t*)(buf + 8);
    int16_t f3 = *(int16_t*)(buf + 12);
    int16_t f4 = *(int16_t*)(buf + 16);
    int16_t f5 = *(int16_t*)(buf + 20);
    int16_t f6 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
    ZASSERT(f6 == value);
}
static void test1(void)
{
    char* buf = opaque(malloc(26));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 4) = value;
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 12) = value;
    *(int16_t*)(buf + 16) = value;
    *(int16_t*)(buf + 20) = value;
    *(int16_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 4);
    int16_t f2 = *(int16_t*)(buf + 8);
    int16_t f3 = *(int16_t*)(buf + 12);
    int16_t f4 = *(int16_t*)(buf + 16);
    int16_t f5 = *(int16_t*)(buf + 20);
    int16_t f6 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
    ZASSERT(f6 == value);
}
static void test2(void)
{
    char* buf = (malloc(26));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 4) = value;
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 12) = value;
    *(int16_t*)(buf + 16) = value;
    *(int16_t*)(buf + 20) = value;
    *(int16_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 4);
    int16_t f2 = *(int16_t*)(buf + 8);
    int16_t f3 = *(int16_t*)(buf + 12);
    int16_t f4 = *(int16_t*)(buf + 16);
    int16_t f5 = *(int16_t*)(buf + 20);
    int16_t f6 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
    ZASSERT(f6 == value);
}
static void test3(void)
{
    char* buf = (malloc(26));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 4) = value;
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 12) = value;
    *(int16_t*)(buf + 16) = value;
    *(int16_t*)(buf + 20) = value;
    *(int16_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 4);
    int16_t f2 = *(int16_t*)(buf + 8);
    int16_t f3 = *(int16_t*)(buf + 12);
    int16_t f4 = *(int16_t*)(buf + 16);
    int16_t f5 = *(int16_t*)(buf + 20);
    int16_t f6 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
    ZASSERT(f6 == value);
}
static void test4(void)
{
    char* buf = opaque(malloc(24));
    *(int16_t*)(buf + 2) = value;
    *(int16_t*)(buf + 6) = value;
    *(int16_t*)(buf + 10) = value;
    *(int16_t*)(buf + 14) = value;
    *(int16_t*)(buf + 18) = value;
    *(int16_t*)(buf + 22) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 2);
    int16_t f1 = *(int16_t*)(buf + 6);
    int16_t f2 = *(int16_t*)(buf + 10);
    int16_t f3 = *(int16_t*)(buf + 14);
    int16_t f4 = *(int16_t*)(buf + 18);
    int16_t f5 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
}
static void test5(void)
{
    char* buf = opaque(malloc(24));
    *(int16_t*)(buf + 2) = value;
    *(int16_t*)(buf + 6) = value;
    *(int16_t*)(buf + 10) = value;
    *(int16_t*)(buf + 14) = value;
    *(int16_t*)(buf + 18) = value;
    *(int16_t*)(buf + 22) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 2);
    int16_t f1 = *(int16_t*)(buf + 6);
    int16_t f2 = *(int16_t*)(buf + 10);
    int16_t f3 = *(int16_t*)(buf + 14);
    int16_t f4 = *(int16_t*)(buf + 18);
    int16_t f5 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
}
static void test6(void)
{
    char* buf = (malloc(24));
    *(int16_t*)(buf + 2) = value;
    *(int16_t*)(buf + 6) = value;
    *(int16_t*)(buf + 10) = value;
    *(int16_t*)(buf + 14) = value;
    *(int16_t*)(buf + 18) = value;
    *(int16_t*)(buf + 22) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 2);
    int16_t f1 = *(int16_t*)(buf + 6);
    int16_t f2 = *(int16_t*)(buf + 10);
    int16_t f3 = *(int16_t*)(buf + 14);
    int16_t f4 = *(int16_t*)(buf + 18);
    int16_t f5 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
}
static void test7(void)
{
    char* buf = (malloc(24));
    *(int16_t*)(buf + 2) = value;
    *(int16_t*)(buf + 6) = value;
    *(int16_t*)(buf + 10) = value;
    *(int16_t*)(buf + 14) = value;
    *(int16_t*)(buf + 18) = value;
    *(int16_t*)(buf + 22) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 2);
    int16_t f1 = *(int16_t*)(buf + 6);
    int16_t f2 = *(int16_t*)(buf + 10);
    int16_t f3 = *(int16_t*)(buf + 14);
    int16_t f4 = *(int16_t*)(buf + 18);
    int16_t f5 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
}
static void test8(void)
{
    char* buf = opaque(malloc(26));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 16) = value;
    *(int16_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 8);
    int16_t f2 = *(int16_t*)(buf + 16);
    int16_t f3 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
}
static void test9(void)
{
    char* buf = opaque(malloc(26));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 16) = value;
    *(int16_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 8);
    int16_t f2 = *(int16_t*)(buf + 16);
    int16_t f3 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
}
static void test10(void)
{
    char* buf = (malloc(26));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 16) = value;
    *(int16_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 8);
    int16_t f2 = *(int16_t*)(buf + 16);
    int16_t f3 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
}
static void test11(void)
{
    char* buf = (malloc(26));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 16) = value;
    *(int16_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 8);
    int16_t f2 = *(int16_t*)(buf + 16);
    int16_t f3 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
}
static void test12(void)
{
    char* buf = opaque(malloc(24));
    *(int16_t*)(buf + 6) = value;
    *(int16_t*)(buf + 14) = value;
    *(int16_t*)(buf + 22) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 6);
    int16_t f1 = *(int16_t*)(buf + 14);
    int16_t f2 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test13(void)
{
    char* buf = opaque(malloc(24));
    *(int16_t*)(buf + 6) = value;
    *(int16_t*)(buf + 14) = value;
    *(int16_t*)(buf + 22) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 6);
    int16_t f1 = *(int16_t*)(buf + 14);
    int16_t f2 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test14(void)
{
    char* buf = (malloc(24));
    *(int16_t*)(buf + 6) = value;
    *(int16_t*)(buf + 14) = value;
    *(int16_t*)(buf + 22) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 6);
    int16_t f1 = *(int16_t*)(buf + 14);
    int16_t f2 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test15(void)
{
    char* buf = (malloc(24));
    *(int16_t*)(buf + 6) = value;
    *(int16_t*)(buf + 14) = value;
    *(int16_t*)(buf + 22) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 6);
    int16_t f1 = *(int16_t*)(buf + 14);
    int16_t f2 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test16(void)
{
    char* buf = opaque(malloc(22));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 10) = value;
    *(int16_t*)(buf + 20) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 10);
    int16_t f2 = *(int16_t*)(buf + 20);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test17(void)
{
    char* buf = opaque(malloc(22));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 10) = value;
    *(int16_t*)(buf + 20) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 10);
    int16_t f2 = *(int16_t*)(buf + 20);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test18(void)
{
    char* buf = (malloc(22));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 10) = value;
    *(int16_t*)(buf + 20) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 10);
    int16_t f2 = *(int16_t*)(buf + 20);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test19(void)
{
    char* buf = (malloc(22));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 10) = value;
    *(int16_t*)(buf + 20) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 10);
    int16_t f2 = *(int16_t*)(buf + 20);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test20(void)
{
    char* buf = opaque(malloc(20));
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 18) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 8);
    int16_t f1 = *(int16_t*)(buf + 18);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test21(void)
{
    char* buf = opaque(malloc(20));
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 18) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 8);
    int16_t f1 = *(int16_t*)(buf + 18);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test22(void)
{
    char* buf = (malloc(20));
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 18) = value;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 8);
    int16_t f1 = *(int16_t*)(buf + 18);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test23(void)
{
    char* buf = (malloc(20));
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 18) = value;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 8);
    int16_t f1 = *(int16_t*)(buf + 18);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
}
static void test24(void)
{
    char* buf = opaque(malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 4) = value;
    *(int32_t*)(buf + 8) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 16) = value;
    *(int32_t*)(buf + 20) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 4);
    int32_t f2 = *(int32_t*)(buf + 8);
    int32_t f3 = *(int32_t*)(buf + 12);
    int32_t f4 = *(int32_t*)(buf + 16);
    int32_t f5 = *(int32_t*)(buf + 20);
    int32_t f6 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
    ZASSERT(f6 == value);
}
static void test25(void)
{
    char* buf = opaque(malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 4) = value;
    *(int32_t*)(buf + 8) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 16) = value;
    *(int32_t*)(buf + 20) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 4);
    int32_t f2 = *(int32_t*)(buf + 8);
    int32_t f3 = *(int32_t*)(buf + 12);
    int32_t f4 = *(int32_t*)(buf + 16);
    int32_t f5 = *(int32_t*)(buf + 20);
    int32_t f6 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
    ZASSERT(f6 == value);
}
static void test26(void)
{
    char* buf = (malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 4) = value;
    *(int32_t*)(buf + 8) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 16) = value;
    *(int32_t*)(buf + 20) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 4);
    int32_t f2 = *(int32_t*)(buf + 8);
    int32_t f3 = *(int32_t*)(buf + 12);
    int32_t f4 = *(int32_t*)(buf + 16);
    int32_t f5 = *(int32_t*)(buf + 20);
    int32_t f6 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
    ZASSERT(f6 == value);
}
static void test27(void)
{
    char* buf = (malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 4) = value;
    *(int32_t*)(buf + 8) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 16) = value;
    *(int32_t*)(buf + 20) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 4);
    int32_t f2 = *(int32_t*)(buf + 8);
    int32_t f3 = *(int32_t*)(buf + 12);
    int32_t f4 = *(int32_t*)(buf + 16);
    int32_t f5 = *(int32_t*)(buf + 20);
    int32_t f6 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
    ZASSERT(f6 == value);
}
static void test28(void)
{
    char* buf = opaque(malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 8) = value;
    *(int32_t*)(buf + 16) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 8);
    int32_t f2 = *(int32_t*)(buf + 16);
    int32_t f3 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
}
static void test29(void)
{
    char* buf = opaque(malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 8) = value;
    *(int32_t*)(buf + 16) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 8);
    int32_t f2 = *(int32_t*)(buf + 16);
    int32_t f3 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
}
static void test30(void)
{
    char* buf = (malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 8) = value;
    *(int32_t*)(buf + 16) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 8);
    int32_t f2 = *(int32_t*)(buf + 16);
    int32_t f3 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
}
static void test31(void)
{
    char* buf = (malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 8) = value;
    *(int32_t*)(buf + 16) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 8);
    int32_t f2 = *(int32_t*)(buf + 16);
    int32_t f3 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
}
static void test32(void)
{
    char* buf = opaque(malloc(24));
    *(int32_t*)(buf + 4) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 20) = value;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 4);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 20);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test33(void)
{
    char* buf = opaque(malloc(24));
    *(int32_t*)(buf + 4) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 20) = value;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 4);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 20);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test34(void)
{
    char* buf = (malloc(24));
    *(int32_t*)(buf + 4) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 20) = value;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 4);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 20);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test35(void)
{
    char* buf = (malloc(24));
    *(int32_t*)(buf + 4) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 20) = value;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 4);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 20);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test36(void)
{
    char* buf = opaque(malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test37(void)
{
    char* buf = opaque(malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test38(void)
{
    char* buf = (malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
}
static void test39(void)
{
    char* buf = (malloc(28));
    *(int32_t*)(buf + 0) = value;
    *(int32_t*)(buf + 12) = value;
    *(int32_t*)(buf + 24) = value;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
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
    return 0;
}
