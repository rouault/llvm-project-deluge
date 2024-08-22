#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
static void test0(void)
{
    char* buf = opaque(malloc(33));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 1) = 42;
    *(int8_t*)(buf + 2) = 42;
    *(int8_t*)(buf + 3) = 42;
    *(int8_t*)(buf + 4) = 42;
    *(int8_t*)(buf + 5) = 42;
    *(int8_t*)(buf + 6) = 42;
    *(int8_t*)(buf + 7) = 42;
    *(int8_t*)(buf + 8) = 42;
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 10) = 42;
    *(int8_t*)(buf + 11) = 42;
    *(int8_t*)(buf + 12) = 42;
    *(int8_t*)(buf + 13) = 42;
    *(int8_t*)(buf + 14) = 42;
    *(int8_t*)(buf + 15) = 42;
    *(int8_t*)(buf + 16) = 42;
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 18) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 20) = 42;
    *(int8_t*)(buf + 21) = 42;
    *(int8_t*)(buf + 22) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 24) = 42;
    *(int8_t*)(buf + 25) = 42;
    *(int8_t*)(buf + 26) = 42;
    *(int8_t*)(buf + 27) = 42;
    *(int8_t*)(buf + 28) = 42;
    *(int8_t*)(buf + 29) = 42;
    *(int8_t*)(buf + 30) = 42;
    *(int8_t*)(buf + 31) = 42;
    *(int8_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 0) == 42);
    ZASSERT(*(int8_t*)(buf + 1) == 42);
    ZASSERT(*(int8_t*)(buf + 2) == 42);
    ZASSERT(*(int8_t*)(buf + 3) == 42);
    ZASSERT(*(int8_t*)(buf + 4) == 42);
    ZASSERT(*(int8_t*)(buf + 5) == 42);
    ZASSERT(*(int8_t*)(buf + 6) == 42);
    ZASSERT(*(int8_t*)(buf + 7) == 42);
    ZASSERT(*(int8_t*)(buf + 8) == 42);
    ZASSERT(*(int8_t*)(buf + 9) == 42);
    ZASSERT(*(int8_t*)(buf + 10) == 42);
    ZASSERT(*(int8_t*)(buf + 11) == 42);
    ZASSERT(*(int8_t*)(buf + 12) == 42);
    ZASSERT(*(int8_t*)(buf + 13) == 42);
    ZASSERT(*(int8_t*)(buf + 14) == 42);
    ZASSERT(*(int8_t*)(buf + 15) == 42);
    ZASSERT(*(int8_t*)(buf + 16) == 42);
    ZASSERT(*(int8_t*)(buf + 17) == 42);
    ZASSERT(*(int8_t*)(buf + 18) == 42);
    ZASSERT(*(int8_t*)(buf + 19) == 42);
    ZASSERT(*(int8_t*)(buf + 20) == 42);
    ZASSERT(*(int8_t*)(buf + 21) == 42);
    ZASSERT(*(int8_t*)(buf + 22) == 42);
    ZASSERT(*(int8_t*)(buf + 23) == 42);
    ZASSERT(*(int8_t*)(buf + 24) == 42);
    ZASSERT(*(int8_t*)(buf + 25) == 42);
    ZASSERT(*(int8_t*)(buf + 26) == 42);
    ZASSERT(*(int8_t*)(buf + 27) == 42);
    ZASSERT(*(int8_t*)(buf + 28) == 42);
    ZASSERT(*(int8_t*)(buf + 29) == 42);
    ZASSERT(*(int8_t*)(buf + 30) == 42);
    ZASSERT(*(int8_t*)(buf + 31) == 42);
    ZASSERT(*(int8_t*)(buf + 32) == 42);
}
static void test1(void)
{
    char* buf = opaque(malloc(33));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 2) = 42;
    *(int8_t*)(buf + 4) = 42;
    *(int8_t*)(buf + 6) = 42;
    *(int8_t*)(buf + 8) = 42;
    *(int8_t*)(buf + 10) = 42;
    *(int8_t*)(buf + 12) = 42;
    *(int8_t*)(buf + 14) = 42;
    *(int8_t*)(buf + 16) = 42;
    *(int8_t*)(buf + 18) = 42;
    *(int8_t*)(buf + 20) = 42;
    *(int8_t*)(buf + 22) = 42;
    *(int8_t*)(buf + 24) = 42;
    *(int8_t*)(buf + 26) = 42;
    *(int8_t*)(buf + 28) = 42;
    *(int8_t*)(buf + 30) = 42;
    *(int8_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 0) == 42);
    ZASSERT(*(int8_t*)(buf + 2) == 42);
    ZASSERT(*(int8_t*)(buf + 4) == 42);
    ZASSERT(*(int8_t*)(buf + 6) == 42);
    ZASSERT(*(int8_t*)(buf + 8) == 42);
    ZASSERT(*(int8_t*)(buf + 10) == 42);
    ZASSERT(*(int8_t*)(buf + 12) == 42);
    ZASSERT(*(int8_t*)(buf + 14) == 42);
    ZASSERT(*(int8_t*)(buf + 16) == 42);
    ZASSERT(*(int8_t*)(buf + 18) == 42);
    ZASSERT(*(int8_t*)(buf + 20) == 42);
    ZASSERT(*(int8_t*)(buf + 22) == 42);
    ZASSERT(*(int8_t*)(buf + 24) == 42);
    ZASSERT(*(int8_t*)(buf + 26) == 42);
    ZASSERT(*(int8_t*)(buf + 28) == 42);
    ZASSERT(*(int8_t*)(buf + 30) == 42);
    ZASSERT(*(int8_t*)(buf + 32) == 42);
}
static void test2(void)
{
    char* buf = opaque(malloc(32));
    *(int8_t*)(buf + 1) = 42;
    *(int8_t*)(buf + 3) = 42;
    *(int8_t*)(buf + 5) = 42;
    *(int8_t*)(buf + 7) = 42;
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 11) = 42;
    *(int8_t*)(buf + 13) = 42;
    *(int8_t*)(buf + 15) = 42;
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 21) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 25) = 42;
    *(int8_t*)(buf + 27) = 42;
    *(int8_t*)(buf + 29) = 42;
    *(int8_t*)(buf + 31) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 1) == 42);
    ZASSERT(*(int8_t*)(buf + 3) == 42);
    ZASSERT(*(int8_t*)(buf + 5) == 42);
    ZASSERT(*(int8_t*)(buf + 7) == 42);
    ZASSERT(*(int8_t*)(buf + 9) == 42);
    ZASSERT(*(int8_t*)(buf + 11) == 42);
    ZASSERT(*(int8_t*)(buf + 13) == 42);
    ZASSERT(*(int8_t*)(buf + 15) == 42);
    ZASSERT(*(int8_t*)(buf + 17) == 42);
    ZASSERT(*(int8_t*)(buf + 19) == 42);
    ZASSERT(*(int8_t*)(buf + 21) == 42);
    ZASSERT(*(int8_t*)(buf + 23) == 42);
    ZASSERT(*(int8_t*)(buf + 25) == 42);
    ZASSERT(*(int8_t*)(buf + 27) == 42);
    ZASSERT(*(int8_t*)(buf + 29) == 42);
    ZASSERT(*(int8_t*)(buf + 31) == 42);
}
static void test3(void)
{
    char* buf = opaque(malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 6) = 42;
    *(int8_t*)(buf + 12) = 42;
    *(int8_t*)(buf + 18) = 42;
    *(int8_t*)(buf + 24) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 0) == 42);
    ZASSERT(*(int8_t*)(buf + 6) == 42);
    ZASSERT(*(int8_t*)(buf + 12) == 42);
    ZASSERT(*(int8_t*)(buf + 18) == 42);
    ZASSERT(*(int8_t*)(buf + 24) == 42);
    ZASSERT(*(int8_t*)(buf + 30) == 42);
}
static void test4(void)
{
    char* buf = opaque(malloc(30));
    *(int8_t*)(buf + 5) = 42;
    *(int8_t*)(buf + 11) = 42;
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 5) == 42);
    ZASSERT(*(int8_t*)(buf + 11) == 42);
    ZASSERT(*(int8_t*)(buf + 17) == 42);
    ZASSERT(*(int8_t*)(buf + 23) == 42);
    ZASSERT(*(int8_t*)(buf + 29) == 42);
}
static void test5(void)
{
    char* buf = opaque(malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 10) = 42;
    *(int8_t*)(buf + 20) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 0) == 42);
    ZASSERT(*(int8_t*)(buf + 10) == 42);
    ZASSERT(*(int8_t*)(buf + 20) == 42);
    ZASSERT(*(int8_t*)(buf + 30) == 42);
}
static void test6(void)
{
    char* buf = opaque(malloc(30));
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 9) == 42);
    ZASSERT(*(int8_t*)(buf + 19) == 42);
    ZASSERT(*(int8_t*)(buf + 29) == 42);
}
static void test7(void)
{
    char* buf = opaque(malloc(18));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 17) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 0) == 42);
    ZASSERT(*(int8_t*)(buf + 17) == 42);
}
static void test8(void)
{
    char* buf = opaque(malloc(17));
    *(int8_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int8_t*)(buf + 16) == 42);
}
static void test9(void)
{
    char* buf = opaque(malloc(34));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 2) = 42;
    *(int16_t*)(buf + 4) = 42;
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 8) = 42;
    *(int16_t*)(buf + 10) = 42;
    *(int16_t*)(buf + 12) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 16) = 42;
    *(int16_t*)(buf + 18) = 42;
    *(int16_t*)(buf + 20) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 24) = 42;
    *(int16_t*)(buf + 26) = 42;
    *(int16_t*)(buf + 28) = 42;
    *(int16_t*)(buf + 30) = 42;
    *(int16_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int16_t*)(buf + 0) == 42);
    ZASSERT(*(int16_t*)(buf + 2) == 42);
    ZASSERT(*(int16_t*)(buf + 4) == 42);
    ZASSERT(*(int16_t*)(buf + 6) == 42);
    ZASSERT(*(int16_t*)(buf + 8) == 42);
    ZASSERT(*(int16_t*)(buf + 10) == 42);
    ZASSERT(*(int16_t*)(buf + 12) == 42);
    ZASSERT(*(int16_t*)(buf + 14) == 42);
    ZASSERT(*(int16_t*)(buf + 16) == 42);
    ZASSERT(*(int16_t*)(buf + 18) == 42);
    ZASSERT(*(int16_t*)(buf + 20) == 42);
    ZASSERT(*(int16_t*)(buf + 22) == 42);
    ZASSERT(*(int16_t*)(buf + 24) == 42);
    ZASSERT(*(int16_t*)(buf + 26) == 42);
    ZASSERT(*(int16_t*)(buf + 28) == 42);
    ZASSERT(*(int16_t*)(buf + 30) == 42);
    ZASSERT(*(int16_t*)(buf + 32) == 42);
}
static void test10(void)
{
    char* buf = opaque(malloc(34));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 4) = 42;
    *(int16_t*)(buf + 8) = 42;
    *(int16_t*)(buf + 12) = 42;
    *(int16_t*)(buf + 16) = 42;
    *(int16_t*)(buf + 20) = 42;
    *(int16_t*)(buf + 24) = 42;
    *(int16_t*)(buf + 28) = 42;
    *(int16_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int16_t*)(buf + 0) == 42);
    ZASSERT(*(int16_t*)(buf + 4) == 42);
    ZASSERT(*(int16_t*)(buf + 8) == 42);
    ZASSERT(*(int16_t*)(buf + 12) == 42);
    ZASSERT(*(int16_t*)(buf + 16) == 42);
    ZASSERT(*(int16_t*)(buf + 20) == 42);
    ZASSERT(*(int16_t*)(buf + 24) == 42);
    ZASSERT(*(int16_t*)(buf + 28) == 42);
    ZASSERT(*(int16_t*)(buf + 32) == 42);
}
static void test11(void)
{
    char* buf = opaque(malloc(32));
    *(int16_t*)(buf + 2) = 42;
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 10) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 18) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 26) = 42;
    *(int16_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int16_t*)(buf + 2) == 42);
    ZASSERT(*(int16_t*)(buf + 6) == 42);
    ZASSERT(*(int16_t*)(buf + 10) == 42);
    ZASSERT(*(int16_t*)(buf + 14) == 42);
    ZASSERT(*(int16_t*)(buf + 18) == 42);
    ZASSERT(*(int16_t*)(buf + 22) == 42);
    ZASSERT(*(int16_t*)(buf + 26) == 42);
    ZASSERT(*(int16_t*)(buf + 30) == 42);
}
static void test12(void)
{
    char* buf = opaque(malloc(34));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 8) = 42;
    *(int16_t*)(buf + 16) = 42;
    *(int16_t*)(buf + 24) = 42;
    *(int16_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int16_t*)(buf + 0) == 42);
    ZASSERT(*(int16_t*)(buf + 8) == 42);
    ZASSERT(*(int16_t*)(buf + 16) == 42);
    ZASSERT(*(int16_t*)(buf + 24) == 42);
    ZASSERT(*(int16_t*)(buf + 32) == 42);
}
static void test13(void)
{
    char* buf = opaque(malloc(32));
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int16_t*)(buf + 6) == 42);
    ZASSERT(*(int16_t*)(buf + 14) == 42);
    ZASSERT(*(int16_t*)(buf + 22) == 42);
    ZASSERT(*(int16_t*)(buf + 30) == 42);
}
static void test14(void)
{
    char* buf = opaque(malloc(26));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 12) = 42;
    *(int16_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int16_t*)(buf + 0) == 42);
    ZASSERT(*(int16_t*)(buf + 12) == 42);
    ZASSERT(*(int16_t*)(buf + 24) == 42);
}
static void test15(void)
{
    char* buf = opaque(malloc(24));
    *(int16_t*)(buf + 10) = 42;
    *(int16_t*)(buf + 22) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int16_t*)(buf + 10) == 42);
    ZASSERT(*(int16_t*)(buf + 22) == 42);
}
static void test16(void)
{
    char* buf = opaque(malloc(20));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 18) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int16_t*)(buf + 0) == 42);
    ZASSERT(*(int16_t*)(buf + 18) == 42);
}
static void test17(void)
{
    char* buf = opaque(malloc(18));
    *(int16_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int16_t*)(buf + 16) == 42);
}
static void test18(void)
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 4) = 42;
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 24) = 42;
    *(int32_t*)(buf + 28) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 0) == 42);
    ZASSERT(*(int32_t*)(buf + 4) == 42);
    ZASSERT(*(int32_t*)(buf + 8) == 42);
    ZASSERT(*(int32_t*)(buf + 12) == 42);
    ZASSERT(*(int32_t*)(buf + 16) == 42);
    ZASSERT(*(int32_t*)(buf + 20) == 42);
    ZASSERT(*(int32_t*)(buf + 24) == 42);
    ZASSERT(*(int32_t*)(buf + 28) == 42);
    ZASSERT(*(int32_t*)(buf + 32) == 42);
}
static void test19(void)
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 24) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 0) == 42);
    ZASSERT(*(int32_t*)(buf + 8) == 42);
    ZASSERT(*(int32_t*)(buf + 16) == 42);
    ZASSERT(*(int32_t*)(buf + 24) == 42);
    ZASSERT(*(int32_t*)(buf + 32) == 42);
}
static void test20(void)
{
    char* buf = opaque(malloc(32));
    *(int32_t*)(buf + 4) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 4) == 42);
    ZASSERT(*(int32_t*)(buf + 12) == 42);
    ZASSERT(*(int32_t*)(buf + 20) == 42);
    ZASSERT(*(int32_t*)(buf + 28) == 42);
}
static void test21(void)
{
    char* buf = opaque(malloc(28));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 0) == 42);
    ZASSERT(*(int32_t*)(buf + 12) == 42);
    ZASSERT(*(int32_t*)(buf + 24) == 42);
}
static void test22(void)
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 8) == 42);
    ZASSERT(*(int32_t*)(buf + 20) == 42);
    ZASSERT(*(int32_t*)(buf + 32) == 42);
}
static void test23(void)
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 0) == 42);
    ZASSERT(*(int32_t*)(buf + 16) == 42);
    ZASSERT(*(int32_t*)(buf + 32) == 42);
}
static void test24(void)
{
    char* buf = opaque(malloc(32));
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 12) == 42);
    ZASSERT(*(int32_t*)(buf + 28) == 42);
}
static void test25(void)
{
    char* buf = opaque(malloc(24));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 20) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 0) == 42);
    ZASSERT(*(int32_t*)(buf + 20) == 42);
}
static void test26(void)
{
    char* buf = opaque(malloc(20));
    *(int32_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int32_t*)(buf + 16) == 42);
}
static void test27(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test28(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test29(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test30(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test31(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
}
static void test32(void)
{
    char* buf = opaque(malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 16) = 42;
    *(__int128*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(__int128*)(buf + 0) == 42);
    ZASSERT(*(__int128*)(buf + 16) == 42);
    ZASSERT(*(__int128*)(buf + 32) == 42);
}
static void test33(void)
{
    char* buf = opaque(malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(__int128*)(buf + 0) == 42);
    ZASSERT(*(__int128*)(buf + 32) == 42);
}
static void test34(void)
{
    char* buf = opaque(malloc(32));
    *(__int128*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(__int128*)(buf + 16) == 42);
}
static void test35(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test36(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test37(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test38(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test39(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test40(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test41(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test42(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test43(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
}
static void test44(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test45(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test46(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test47(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test48(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
}
static void test49(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test50(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test51(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test52(void)
{
    char* buf = opaque(malloc(16));
    *(int64_t*)(buf + 8) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
}
static void test53(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test54(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test55(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
}
static void test56(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test57(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test58(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test59(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
}
static void test60(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test61(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test62(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
}
static void test63(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test64(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test65(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test66(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test67(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
}
static void test68(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
}
static void test69(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test70(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test71(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test72(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 8) == 42);
}
static void test73(void)
{
    char* buf = opaque(malloc(8));
    *(int64_t*)(buf + 0) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
}
static void test74(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test75(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test76(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
}
static void test77(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test78(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test79(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test80(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
}
static void test81(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test82(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
}
static void test83(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test84(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test85(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test86(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test87(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
}
static void test88(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
}
static void test89(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test90(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test91(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test92(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
}
static void test93(void)
{
    char* buf = opaque(malloc(16));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
}
static void test94(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test95(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test96(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
}
static void test97(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test98(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test99(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test100(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test101(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
}
static void test102(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test103(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test104(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
}
static void test105(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test106(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test107(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test108(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
}
static void test109(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
}
static void test110(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test111(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test112(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test113(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 0) == 42);
    ZASSERT(*(int64_t*)(buf + 8) == 42);
}
static void test114(void)
{
    char* buf = opaque(malloc(16));
    *(char**)(buf + 0) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
}
static void test115(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test116(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test117(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test118(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test119(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test120(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test121(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test122(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test123(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test124(void)
{
    char* buf = opaque(malloc(24));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 16) == 42);
}
static void test125(void)
{
    char* buf = opaque(malloc(24));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
}
static void test126(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test127(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test128(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test129(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
}
static void test130(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test131(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test132(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test133(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test134(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
static void test135(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 16) == 42);
    ZASSERT(*(int64_t*)(buf + 24) == 42);
}
static void test136(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
}
static void test137(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test138(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(int64_t*)(buf + 32) == 42);
}
static void test139(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(!strcmp(*(char**)(buf + 16), "hello"));
    ZASSERT(!strcmp(*(char**)(buf + 32), "hello"));
}
int main()
{
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
    test64();
    test65();
    test66();
    test67();
    test68();
    test69();
    test70();
    test71();
    test72();
    test73();
    test74();
    test75();
    test76();
    test77();
    test78();
    test79();
    test80();
    test81();
    test82();
    test83();
    test84();
    test85();
    test86();
    test87();
    test88();
    test89();
    test90();
    test91();
    test92();
    test93();
    test94();
    test95();
    test96();
    test97();
    test98();
    test99();
    test100();
    test101();
    test102();
    test103();
    test104();
    test105();
    test106();
    test107();
    test108();
    test109();
    test110();
    test111();
    test112();
    test113();
    test114();
    test115();
    test116();
    test117();
    test118();
    test119();
    test120();
    test121();
    test122();
    test123();
    test124();
    test125();
    test126();
    test127();
    test128();
    test129();
    test130();
    test131();
    test132();
    test133();
    test134();
    test135();
    test136();
    test137();
    test138();
    test139();
    return 0;
}
