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
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 1);
    int8_t f2 = *(int8_t*)(buf + 2);
    int8_t f3 = *(int8_t*)(buf + 3);
    int8_t f4 = *(int8_t*)(buf + 4);
    int8_t f5 = *(int8_t*)(buf + 5);
    int8_t f6 = *(int8_t*)(buf + 6);
    int8_t f7 = *(int8_t*)(buf + 7);
    int8_t f8 = *(int8_t*)(buf + 8);
    int8_t f9 = *(int8_t*)(buf + 9);
    int8_t f10 = *(int8_t*)(buf + 10);
    int8_t f11 = *(int8_t*)(buf + 11);
    int8_t f12 = *(int8_t*)(buf + 12);
    int8_t f13 = *(int8_t*)(buf + 13);
    int8_t f14 = *(int8_t*)(buf + 14);
    int8_t f15 = *(int8_t*)(buf + 15);
    int8_t f16 = *(int8_t*)(buf + 16);
    int8_t f17 = *(int8_t*)(buf + 17);
    int8_t f18 = *(int8_t*)(buf + 18);
    int8_t f19 = *(int8_t*)(buf + 19);
    int8_t f20 = *(int8_t*)(buf + 20);
    int8_t f21 = *(int8_t*)(buf + 21);
    int8_t f22 = *(int8_t*)(buf + 22);
    int8_t f23 = *(int8_t*)(buf + 23);
    int8_t f24 = *(int8_t*)(buf + 24);
    int8_t f25 = *(int8_t*)(buf + 25);
    int8_t f26 = *(int8_t*)(buf + 26);
    int8_t f27 = *(int8_t*)(buf + 27);
    int8_t f28 = *(int8_t*)(buf + 28);
    int8_t f29 = *(int8_t*)(buf + 29);
    int8_t f30 = *(int8_t*)(buf + 30);
    int8_t f31 = *(int8_t*)(buf + 31);
    int8_t f32 = *(int8_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
    ZASSERT(f17 == 42);
    ZASSERT(f18 == 42);
    ZASSERT(f19 == 42);
    ZASSERT(f20 == 42);
    ZASSERT(f21 == 42);
    ZASSERT(f22 == 42);
    ZASSERT(f23 == 42);
    ZASSERT(f24 == 42);
    ZASSERT(f25 == 42);
    ZASSERT(f26 == 42);
    ZASSERT(f27 == 42);
    ZASSERT(f28 == 42);
    ZASSERT(f29 == 42);
    ZASSERT(f30 == 42);
    ZASSERT(f31 == 42);
    ZASSERT(f32 == 42);
}
static void test1(void)
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
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 1);
    int8_t f2 = *(int8_t*)(buf + 2);
    int8_t f3 = *(int8_t*)(buf + 3);
    int8_t f4 = *(int8_t*)(buf + 4);
    int8_t f5 = *(int8_t*)(buf + 5);
    int8_t f6 = *(int8_t*)(buf + 6);
    int8_t f7 = *(int8_t*)(buf + 7);
    int8_t f8 = *(int8_t*)(buf + 8);
    int8_t f9 = *(int8_t*)(buf + 9);
    int8_t f10 = *(int8_t*)(buf + 10);
    int8_t f11 = *(int8_t*)(buf + 11);
    int8_t f12 = *(int8_t*)(buf + 12);
    int8_t f13 = *(int8_t*)(buf + 13);
    int8_t f14 = *(int8_t*)(buf + 14);
    int8_t f15 = *(int8_t*)(buf + 15);
    int8_t f16 = *(int8_t*)(buf + 16);
    int8_t f17 = *(int8_t*)(buf + 17);
    int8_t f18 = *(int8_t*)(buf + 18);
    int8_t f19 = *(int8_t*)(buf + 19);
    int8_t f20 = *(int8_t*)(buf + 20);
    int8_t f21 = *(int8_t*)(buf + 21);
    int8_t f22 = *(int8_t*)(buf + 22);
    int8_t f23 = *(int8_t*)(buf + 23);
    int8_t f24 = *(int8_t*)(buf + 24);
    int8_t f25 = *(int8_t*)(buf + 25);
    int8_t f26 = *(int8_t*)(buf + 26);
    int8_t f27 = *(int8_t*)(buf + 27);
    int8_t f28 = *(int8_t*)(buf + 28);
    int8_t f29 = *(int8_t*)(buf + 29);
    int8_t f30 = *(int8_t*)(buf + 30);
    int8_t f31 = *(int8_t*)(buf + 31);
    int8_t f32 = *(int8_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
    ZASSERT(f17 == 42);
    ZASSERT(f18 == 42);
    ZASSERT(f19 == 42);
    ZASSERT(f20 == 42);
    ZASSERT(f21 == 42);
    ZASSERT(f22 == 42);
    ZASSERT(f23 == 42);
    ZASSERT(f24 == 42);
    ZASSERT(f25 == 42);
    ZASSERT(f26 == 42);
    ZASSERT(f27 == 42);
    ZASSERT(f28 == 42);
    ZASSERT(f29 == 42);
    ZASSERT(f30 == 42);
    ZASSERT(f31 == 42);
    ZASSERT(f32 == 42);
}
static void test2(void)
{
    char* buf = (malloc(33));
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
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 1);
    int8_t f2 = *(int8_t*)(buf + 2);
    int8_t f3 = *(int8_t*)(buf + 3);
    int8_t f4 = *(int8_t*)(buf + 4);
    int8_t f5 = *(int8_t*)(buf + 5);
    int8_t f6 = *(int8_t*)(buf + 6);
    int8_t f7 = *(int8_t*)(buf + 7);
    int8_t f8 = *(int8_t*)(buf + 8);
    int8_t f9 = *(int8_t*)(buf + 9);
    int8_t f10 = *(int8_t*)(buf + 10);
    int8_t f11 = *(int8_t*)(buf + 11);
    int8_t f12 = *(int8_t*)(buf + 12);
    int8_t f13 = *(int8_t*)(buf + 13);
    int8_t f14 = *(int8_t*)(buf + 14);
    int8_t f15 = *(int8_t*)(buf + 15);
    int8_t f16 = *(int8_t*)(buf + 16);
    int8_t f17 = *(int8_t*)(buf + 17);
    int8_t f18 = *(int8_t*)(buf + 18);
    int8_t f19 = *(int8_t*)(buf + 19);
    int8_t f20 = *(int8_t*)(buf + 20);
    int8_t f21 = *(int8_t*)(buf + 21);
    int8_t f22 = *(int8_t*)(buf + 22);
    int8_t f23 = *(int8_t*)(buf + 23);
    int8_t f24 = *(int8_t*)(buf + 24);
    int8_t f25 = *(int8_t*)(buf + 25);
    int8_t f26 = *(int8_t*)(buf + 26);
    int8_t f27 = *(int8_t*)(buf + 27);
    int8_t f28 = *(int8_t*)(buf + 28);
    int8_t f29 = *(int8_t*)(buf + 29);
    int8_t f30 = *(int8_t*)(buf + 30);
    int8_t f31 = *(int8_t*)(buf + 31);
    int8_t f32 = *(int8_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
    ZASSERT(f17 == 42);
    ZASSERT(f18 == 42);
    ZASSERT(f19 == 42);
    ZASSERT(f20 == 42);
    ZASSERT(f21 == 42);
    ZASSERT(f22 == 42);
    ZASSERT(f23 == 42);
    ZASSERT(f24 == 42);
    ZASSERT(f25 == 42);
    ZASSERT(f26 == 42);
    ZASSERT(f27 == 42);
    ZASSERT(f28 == 42);
    ZASSERT(f29 == 42);
    ZASSERT(f30 == 42);
    ZASSERT(f31 == 42);
    ZASSERT(f32 == 42);
}
static void test3(void)
{
    char* buf = (malloc(33));
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
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 1);
    int8_t f2 = *(int8_t*)(buf + 2);
    int8_t f3 = *(int8_t*)(buf + 3);
    int8_t f4 = *(int8_t*)(buf + 4);
    int8_t f5 = *(int8_t*)(buf + 5);
    int8_t f6 = *(int8_t*)(buf + 6);
    int8_t f7 = *(int8_t*)(buf + 7);
    int8_t f8 = *(int8_t*)(buf + 8);
    int8_t f9 = *(int8_t*)(buf + 9);
    int8_t f10 = *(int8_t*)(buf + 10);
    int8_t f11 = *(int8_t*)(buf + 11);
    int8_t f12 = *(int8_t*)(buf + 12);
    int8_t f13 = *(int8_t*)(buf + 13);
    int8_t f14 = *(int8_t*)(buf + 14);
    int8_t f15 = *(int8_t*)(buf + 15);
    int8_t f16 = *(int8_t*)(buf + 16);
    int8_t f17 = *(int8_t*)(buf + 17);
    int8_t f18 = *(int8_t*)(buf + 18);
    int8_t f19 = *(int8_t*)(buf + 19);
    int8_t f20 = *(int8_t*)(buf + 20);
    int8_t f21 = *(int8_t*)(buf + 21);
    int8_t f22 = *(int8_t*)(buf + 22);
    int8_t f23 = *(int8_t*)(buf + 23);
    int8_t f24 = *(int8_t*)(buf + 24);
    int8_t f25 = *(int8_t*)(buf + 25);
    int8_t f26 = *(int8_t*)(buf + 26);
    int8_t f27 = *(int8_t*)(buf + 27);
    int8_t f28 = *(int8_t*)(buf + 28);
    int8_t f29 = *(int8_t*)(buf + 29);
    int8_t f30 = *(int8_t*)(buf + 30);
    int8_t f31 = *(int8_t*)(buf + 31);
    int8_t f32 = *(int8_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
    ZASSERT(f17 == 42);
    ZASSERT(f18 == 42);
    ZASSERT(f19 == 42);
    ZASSERT(f20 == 42);
    ZASSERT(f21 == 42);
    ZASSERT(f22 == 42);
    ZASSERT(f23 == 42);
    ZASSERT(f24 == 42);
    ZASSERT(f25 == 42);
    ZASSERT(f26 == 42);
    ZASSERT(f27 == 42);
    ZASSERT(f28 == 42);
    ZASSERT(f29 == 42);
    ZASSERT(f30 == 42);
    ZASSERT(f31 == 42);
    ZASSERT(f32 == 42);
}
static void test4(void)
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
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 2);
    int8_t f2 = *(int8_t*)(buf + 4);
    int8_t f3 = *(int8_t*)(buf + 6);
    int8_t f4 = *(int8_t*)(buf + 8);
    int8_t f5 = *(int8_t*)(buf + 10);
    int8_t f6 = *(int8_t*)(buf + 12);
    int8_t f7 = *(int8_t*)(buf + 14);
    int8_t f8 = *(int8_t*)(buf + 16);
    int8_t f9 = *(int8_t*)(buf + 18);
    int8_t f10 = *(int8_t*)(buf + 20);
    int8_t f11 = *(int8_t*)(buf + 22);
    int8_t f12 = *(int8_t*)(buf + 24);
    int8_t f13 = *(int8_t*)(buf + 26);
    int8_t f14 = *(int8_t*)(buf + 28);
    int8_t f15 = *(int8_t*)(buf + 30);
    int8_t f16 = *(int8_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
}
static void test5(void)
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
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 2);
    int8_t f2 = *(int8_t*)(buf + 4);
    int8_t f3 = *(int8_t*)(buf + 6);
    int8_t f4 = *(int8_t*)(buf + 8);
    int8_t f5 = *(int8_t*)(buf + 10);
    int8_t f6 = *(int8_t*)(buf + 12);
    int8_t f7 = *(int8_t*)(buf + 14);
    int8_t f8 = *(int8_t*)(buf + 16);
    int8_t f9 = *(int8_t*)(buf + 18);
    int8_t f10 = *(int8_t*)(buf + 20);
    int8_t f11 = *(int8_t*)(buf + 22);
    int8_t f12 = *(int8_t*)(buf + 24);
    int8_t f13 = *(int8_t*)(buf + 26);
    int8_t f14 = *(int8_t*)(buf + 28);
    int8_t f15 = *(int8_t*)(buf + 30);
    int8_t f16 = *(int8_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
}
static void test6(void)
{
    char* buf = (malloc(33));
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
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 2);
    int8_t f2 = *(int8_t*)(buf + 4);
    int8_t f3 = *(int8_t*)(buf + 6);
    int8_t f4 = *(int8_t*)(buf + 8);
    int8_t f5 = *(int8_t*)(buf + 10);
    int8_t f6 = *(int8_t*)(buf + 12);
    int8_t f7 = *(int8_t*)(buf + 14);
    int8_t f8 = *(int8_t*)(buf + 16);
    int8_t f9 = *(int8_t*)(buf + 18);
    int8_t f10 = *(int8_t*)(buf + 20);
    int8_t f11 = *(int8_t*)(buf + 22);
    int8_t f12 = *(int8_t*)(buf + 24);
    int8_t f13 = *(int8_t*)(buf + 26);
    int8_t f14 = *(int8_t*)(buf + 28);
    int8_t f15 = *(int8_t*)(buf + 30);
    int8_t f16 = *(int8_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
}
static void test7(void)
{
    char* buf = (malloc(33));
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
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 2);
    int8_t f2 = *(int8_t*)(buf + 4);
    int8_t f3 = *(int8_t*)(buf + 6);
    int8_t f4 = *(int8_t*)(buf + 8);
    int8_t f5 = *(int8_t*)(buf + 10);
    int8_t f6 = *(int8_t*)(buf + 12);
    int8_t f7 = *(int8_t*)(buf + 14);
    int8_t f8 = *(int8_t*)(buf + 16);
    int8_t f9 = *(int8_t*)(buf + 18);
    int8_t f10 = *(int8_t*)(buf + 20);
    int8_t f11 = *(int8_t*)(buf + 22);
    int8_t f12 = *(int8_t*)(buf + 24);
    int8_t f13 = *(int8_t*)(buf + 26);
    int8_t f14 = *(int8_t*)(buf + 28);
    int8_t f15 = *(int8_t*)(buf + 30);
    int8_t f16 = *(int8_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
}
static void test8(void)
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
    int8_t f0 = *(int8_t*)(buf + 1);
    int8_t f1 = *(int8_t*)(buf + 3);
    int8_t f2 = *(int8_t*)(buf + 5);
    int8_t f3 = *(int8_t*)(buf + 7);
    int8_t f4 = *(int8_t*)(buf + 9);
    int8_t f5 = *(int8_t*)(buf + 11);
    int8_t f6 = *(int8_t*)(buf + 13);
    int8_t f7 = *(int8_t*)(buf + 15);
    int8_t f8 = *(int8_t*)(buf + 17);
    int8_t f9 = *(int8_t*)(buf + 19);
    int8_t f10 = *(int8_t*)(buf + 21);
    int8_t f11 = *(int8_t*)(buf + 23);
    int8_t f12 = *(int8_t*)(buf + 25);
    int8_t f13 = *(int8_t*)(buf + 27);
    int8_t f14 = *(int8_t*)(buf + 29);
    int8_t f15 = *(int8_t*)(buf + 31);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
}
static void test9(void)
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
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 1);
    int8_t f1 = *(int8_t*)(buf + 3);
    int8_t f2 = *(int8_t*)(buf + 5);
    int8_t f3 = *(int8_t*)(buf + 7);
    int8_t f4 = *(int8_t*)(buf + 9);
    int8_t f5 = *(int8_t*)(buf + 11);
    int8_t f6 = *(int8_t*)(buf + 13);
    int8_t f7 = *(int8_t*)(buf + 15);
    int8_t f8 = *(int8_t*)(buf + 17);
    int8_t f9 = *(int8_t*)(buf + 19);
    int8_t f10 = *(int8_t*)(buf + 21);
    int8_t f11 = *(int8_t*)(buf + 23);
    int8_t f12 = *(int8_t*)(buf + 25);
    int8_t f13 = *(int8_t*)(buf + 27);
    int8_t f14 = *(int8_t*)(buf + 29);
    int8_t f15 = *(int8_t*)(buf + 31);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
}
static void test10(void)
{
    char* buf = (malloc(32));
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
    int8_t f0 = *(int8_t*)(buf + 1);
    int8_t f1 = *(int8_t*)(buf + 3);
    int8_t f2 = *(int8_t*)(buf + 5);
    int8_t f3 = *(int8_t*)(buf + 7);
    int8_t f4 = *(int8_t*)(buf + 9);
    int8_t f5 = *(int8_t*)(buf + 11);
    int8_t f6 = *(int8_t*)(buf + 13);
    int8_t f7 = *(int8_t*)(buf + 15);
    int8_t f8 = *(int8_t*)(buf + 17);
    int8_t f9 = *(int8_t*)(buf + 19);
    int8_t f10 = *(int8_t*)(buf + 21);
    int8_t f11 = *(int8_t*)(buf + 23);
    int8_t f12 = *(int8_t*)(buf + 25);
    int8_t f13 = *(int8_t*)(buf + 27);
    int8_t f14 = *(int8_t*)(buf + 29);
    int8_t f15 = *(int8_t*)(buf + 31);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
}
static void test11(void)
{
    char* buf = (malloc(32));
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
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 1);
    int8_t f1 = *(int8_t*)(buf + 3);
    int8_t f2 = *(int8_t*)(buf + 5);
    int8_t f3 = *(int8_t*)(buf + 7);
    int8_t f4 = *(int8_t*)(buf + 9);
    int8_t f5 = *(int8_t*)(buf + 11);
    int8_t f6 = *(int8_t*)(buf + 13);
    int8_t f7 = *(int8_t*)(buf + 15);
    int8_t f8 = *(int8_t*)(buf + 17);
    int8_t f9 = *(int8_t*)(buf + 19);
    int8_t f10 = *(int8_t*)(buf + 21);
    int8_t f11 = *(int8_t*)(buf + 23);
    int8_t f12 = *(int8_t*)(buf + 25);
    int8_t f13 = *(int8_t*)(buf + 27);
    int8_t f14 = *(int8_t*)(buf + 29);
    int8_t f15 = *(int8_t*)(buf + 31);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
}
static void test12(void)
{
    char* buf = opaque(malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 6) = 42;
    *(int8_t*)(buf + 12) = 42;
    *(int8_t*)(buf + 18) = 42;
    *(int8_t*)(buf + 24) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 6);
    int8_t f2 = *(int8_t*)(buf + 12);
    int8_t f3 = *(int8_t*)(buf + 18);
    int8_t f4 = *(int8_t*)(buf + 24);
    int8_t f5 = *(int8_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
}
static void test13(void)
{
    char* buf = opaque(malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 6) = 42;
    *(int8_t*)(buf + 12) = 42;
    *(int8_t*)(buf + 18) = 42;
    *(int8_t*)(buf + 24) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 6);
    int8_t f2 = *(int8_t*)(buf + 12);
    int8_t f3 = *(int8_t*)(buf + 18);
    int8_t f4 = *(int8_t*)(buf + 24);
    int8_t f5 = *(int8_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
}
static void test14(void)
{
    char* buf = (malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 6) = 42;
    *(int8_t*)(buf + 12) = 42;
    *(int8_t*)(buf + 18) = 42;
    *(int8_t*)(buf + 24) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 6);
    int8_t f2 = *(int8_t*)(buf + 12);
    int8_t f3 = *(int8_t*)(buf + 18);
    int8_t f4 = *(int8_t*)(buf + 24);
    int8_t f5 = *(int8_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
}
static void test15(void)
{
    char* buf = (malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 6) = 42;
    *(int8_t*)(buf + 12) = 42;
    *(int8_t*)(buf + 18) = 42;
    *(int8_t*)(buf + 24) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 6);
    int8_t f2 = *(int8_t*)(buf + 12);
    int8_t f3 = *(int8_t*)(buf + 18);
    int8_t f4 = *(int8_t*)(buf + 24);
    int8_t f5 = *(int8_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
}
static void test16(void)
{
    char* buf = opaque(malloc(30));
    *(int8_t*)(buf + 5) = 42;
    *(int8_t*)(buf + 11) = 42;
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 5);
    int8_t f1 = *(int8_t*)(buf + 11);
    int8_t f2 = *(int8_t*)(buf + 17);
    int8_t f3 = *(int8_t*)(buf + 23);
    int8_t f4 = *(int8_t*)(buf + 29);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test17(void)
{
    char* buf = opaque(malloc(30));
    *(int8_t*)(buf + 5) = 42;
    *(int8_t*)(buf + 11) = 42;
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 5);
    int8_t f1 = *(int8_t*)(buf + 11);
    int8_t f2 = *(int8_t*)(buf + 17);
    int8_t f3 = *(int8_t*)(buf + 23);
    int8_t f4 = *(int8_t*)(buf + 29);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test18(void)
{
    char* buf = (malloc(30));
    *(int8_t*)(buf + 5) = 42;
    *(int8_t*)(buf + 11) = 42;
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 5);
    int8_t f1 = *(int8_t*)(buf + 11);
    int8_t f2 = *(int8_t*)(buf + 17);
    int8_t f3 = *(int8_t*)(buf + 23);
    int8_t f4 = *(int8_t*)(buf + 29);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test19(void)
{
    char* buf = (malloc(30));
    *(int8_t*)(buf + 5) = 42;
    *(int8_t*)(buf + 11) = 42;
    *(int8_t*)(buf + 17) = 42;
    *(int8_t*)(buf + 23) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 5);
    int8_t f1 = *(int8_t*)(buf + 11);
    int8_t f2 = *(int8_t*)(buf + 17);
    int8_t f3 = *(int8_t*)(buf + 23);
    int8_t f4 = *(int8_t*)(buf + 29);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test20(void)
{
    char* buf = opaque(malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 10) = 42;
    *(int8_t*)(buf + 20) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 10);
    int8_t f2 = *(int8_t*)(buf + 20);
    int8_t f3 = *(int8_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test21(void)
{
    char* buf = opaque(malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 10) = 42;
    *(int8_t*)(buf + 20) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 10);
    int8_t f2 = *(int8_t*)(buf + 20);
    int8_t f3 = *(int8_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test22(void)
{
    char* buf = (malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 10) = 42;
    *(int8_t*)(buf + 20) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 10);
    int8_t f2 = *(int8_t*)(buf + 20);
    int8_t f3 = *(int8_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test23(void)
{
    char* buf = (malloc(31));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 10) = 42;
    *(int8_t*)(buf + 20) = 42;
    *(int8_t*)(buf + 30) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 10);
    int8_t f2 = *(int8_t*)(buf + 20);
    int8_t f3 = *(int8_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test24(void)
{
    char* buf = opaque(malloc(30));
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 9);
    int8_t f1 = *(int8_t*)(buf + 19);
    int8_t f2 = *(int8_t*)(buf + 29);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test25(void)
{
    char* buf = opaque(malloc(30));
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 9);
    int8_t f1 = *(int8_t*)(buf + 19);
    int8_t f2 = *(int8_t*)(buf + 29);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test26(void)
{
    char* buf = (malloc(30));
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 9);
    int8_t f1 = *(int8_t*)(buf + 19);
    int8_t f2 = *(int8_t*)(buf + 29);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test27(void)
{
    char* buf = (malloc(30));
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 9);
    int8_t f1 = *(int8_t*)(buf + 19);
    int8_t f2 = *(int8_t*)(buf + 29);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test28(void)
{
    char* buf = opaque(malloc(18));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 17) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 17);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test29(void)
{
    char* buf = opaque(malloc(18));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 17) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 17);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test30(void)
{
    char* buf = (malloc(18));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 17) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 17);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test31(void)
{
    char* buf = (malloc(18));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 17) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 17);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test32(void)
{
    char* buf = opaque(malloc(17));
    *(int8_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test33(void)
{
    char* buf = opaque(malloc(17));
    *(int8_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test34(void)
{
    char* buf = (malloc(17));
    *(int8_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test35(void)
{
    char* buf = (malloc(17));
    *(int8_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test36(void)
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
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 2);
    int16_t f2 = *(int16_t*)(buf + 4);
    int16_t f3 = *(int16_t*)(buf + 6);
    int16_t f4 = *(int16_t*)(buf + 8);
    int16_t f5 = *(int16_t*)(buf + 10);
    int16_t f6 = *(int16_t*)(buf + 12);
    int16_t f7 = *(int16_t*)(buf + 14);
    int16_t f8 = *(int16_t*)(buf + 16);
    int16_t f9 = *(int16_t*)(buf + 18);
    int16_t f10 = *(int16_t*)(buf + 20);
    int16_t f11 = *(int16_t*)(buf + 22);
    int16_t f12 = *(int16_t*)(buf + 24);
    int16_t f13 = *(int16_t*)(buf + 26);
    int16_t f14 = *(int16_t*)(buf + 28);
    int16_t f15 = *(int16_t*)(buf + 30);
    int16_t f16 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
}
static void test37(void)
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
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 2);
    int16_t f2 = *(int16_t*)(buf + 4);
    int16_t f3 = *(int16_t*)(buf + 6);
    int16_t f4 = *(int16_t*)(buf + 8);
    int16_t f5 = *(int16_t*)(buf + 10);
    int16_t f6 = *(int16_t*)(buf + 12);
    int16_t f7 = *(int16_t*)(buf + 14);
    int16_t f8 = *(int16_t*)(buf + 16);
    int16_t f9 = *(int16_t*)(buf + 18);
    int16_t f10 = *(int16_t*)(buf + 20);
    int16_t f11 = *(int16_t*)(buf + 22);
    int16_t f12 = *(int16_t*)(buf + 24);
    int16_t f13 = *(int16_t*)(buf + 26);
    int16_t f14 = *(int16_t*)(buf + 28);
    int16_t f15 = *(int16_t*)(buf + 30);
    int16_t f16 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
}
static void test38(void)
{
    char* buf = (malloc(34));
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
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 2);
    int16_t f2 = *(int16_t*)(buf + 4);
    int16_t f3 = *(int16_t*)(buf + 6);
    int16_t f4 = *(int16_t*)(buf + 8);
    int16_t f5 = *(int16_t*)(buf + 10);
    int16_t f6 = *(int16_t*)(buf + 12);
    int16_t f7 = *(int16_t*)(buf + 14);
    int16_t f8 = *(int16_t*)(buf + 16);
    int16_t f9 = *(int16_t*)(buf + 18);
    int16_t f10 = *(int16_t*)(buf + 20);
    int16_t f11 = *(int16_t*)(buf + 22);
    int16_t f12 = *(int16_t*)(buf + 24);
    int16_t f13 = *(int16_t*)(buf + 26);
    int16_t f14 = *(int16_t*)(buf + 28);
    int16_t f15 = *(int16_t*)(buf + 30);
    int16_t f16 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
}
static void test39(void)
{
    char* buf = (malloc(34));
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
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 2);
    int16_t f2 = *(int16_t*)(buf + 4);
    int16_t f3 = *(int16_t*)(buf + 6);
    int16_t f4 = *(int16_t*)(buf + 8);
    int16_t f5 = *(int16_t*)(buf + 10);
    int16_t f6 = *(int16_t*)(buf + 12);
    int16_t f7 = *(int16_t*)(buf + 14);
    int16_t f8 = *(int16_t*)(buf + 16);
    int16_t f9 = *(int16_t*)(buf + 18);
    int16_t f10 = *(int16_t*)(buf + 20);
    int16_t f11 = *(int16_t*)(buf + 22);
    int16_t f12 = *(int16_t*)(buf + 24);
    int16_t f13 = *(int16_t*)(buf + 26);
    int16_t f14 = *(int16_t*)(buf + 28);
    int16_t f15 = *(int16_t*)(buf + 30);
    int16_t f16 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
    ZASSERT(f9 == 42);
    ZASSERT(f10 == 42);
    ZASSERT(f11 == 42);
    ZASSERT(f12 == 42);
    ZASSERT(f13 == 42);
    ZASSERT(f14 == 42);
    ZASSERT(f15 == 42);
    ZASSERT(f16 == 42);
}
static void test40(void)
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
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 4);
    int16_t f2 = *(int16_t*)(buf + 8);
    int16_t f3 = *(int16_t*)(buf + 12);
    int16_t f4 = *(int16_t*)(buf + 16);
    int16_t f5 = *(int16_t*)(buf + 20);
    int16_t f6 = *(int16_t*)(buf + 24);
    int16_t f7 = *(int16_t*)(buf + 28);
    int16_t f8 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
}
static void test41(void)
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
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 4);
    int16_t f2 = *(int16_t*)(buf + 8);
    int16_t f3 = *(int16_t*)(buf + 12);
    int16_t f4 = *(int16_t*)(buf + 16);
    int16_t f5 = *(int16_t*)(buf + 20);
    int16_t f6 = *(int16_t*)(buf + 24);
    int16_t f7 = *(int16_t*)(buf + 28);
    int16_t f8 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
}
static void test42(void)
{
    char* buf = (malloc(34));
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
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 4);
    int16_t f2 = *(int16_t*)(buf + 8);
    int16_t f3 = *(int16_t*)(buf + 12);
    int16_t f4 = *(int16_t*)(buf + 16);
    int16_t f5 = *(int16_t*)(buf + 20);
    int16_t f6 = *(int16_t*)(buf + 24);
    int16_t f7 = *(int16_t*)(buf + 28);
    int16_t f8 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
}
static void test43(void)
{
    char* buf = (malloc(34));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 4) = 42;
    *(int16_t*)(buf + 8) = 42;
    *(int16_t*)(buf + 12) = 42;
    *(int16_t*)(buf + 16) = 42;
    *(int16_t*)(buf + 20) = 42;
    *(int16_t*)(buf + 24) = 42;
    *(int16_t*)(buf + 28) = 42;
    *(int16_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 4);
    int16_t f2 = *(int16_t*)(buf + 8);
    int16_t f3 = *(int16_t*)(buf + 12);
    int16_t f4 = *(int16_t*)(buf + 16);
    int16_t f5 = *(int16_t*)(buf + 20);
    int16_t f6 = *(int16_t*)(buf + 24);
    int16_t f7 = *(int16_t*)(buf + 28);
    int16_t f8 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
}
static void test44(void)
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
    int16_t f0 = *(int16_t*)(buf + 2);
    int16_t f1 = *(int16_t*)(buf + 6);
    int16_t f2 = *(int16_t*)(buf + 10);
    int16_t f3 = *(int16_t*)(buf + 14);
    int16_t f4 = *(int16_t*)(buf + 18);
    int16_t f5 = *(int16_t*)(buf + 22);
    int16_t f6 = *(int16_t*)(buf + 26);
    int16_t f7 = *(int16_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
}
static void test45(void)
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
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 2);
    int16_t f1 = *(int16_t*)(buf + 6);
    int16_t f2 = *(int16_t*)(buf + 10);
    int16_t f3 = *(int16_t*)(buf + 14);
    int16_t f4 = *(int16_t*)(buf + 18);
    int16_t f5 = *(int16_t*)(buf + 22);
    int16_t f6 = *(int16_t*)(buf + 26);
    int16_t f7 = *(int16_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
}
static void test46(void)
{
    char* buf = (malloc(32));
    *(int16_t*)(buf + 2) = 42;
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 10) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 18) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 26) = 42;
    *(int16_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 2);
    int16_t f1 = *(int16_t*)(buf + 6);
    int16_t f2 = *(int16_t*)(buf + 10);
    int16_t f3 = *(int16_t*)(buf + 14);
    int16_t f4 = *(int16_t*)(buf + 18);
    int16_t f5 = *(int16_t*)(buf + 22);
    int16_t f6 = *(int16_t*)(buf + 26);
    int16_t f7 = *(int16_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
}
static void test47(void)
{
    char* buf = (malloc(32));
    *(int16_t*)(buf + 2) = 42;
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 10) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 18) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 26) = 42;
    *(int16_t*)(buf + 30) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 2);
    int16_t f1 = *(int16_t*)(buf + 6);
    int16_t f2 = *(int16_t*)(buf + 10);
    int16_t f3 = *(int16_t*)(buf + 14);
    int16_t f4 = *(int16_t*)(buf + 18);
    int16_t f5 = *(int16_t*)(buf + 22);
    int16_t f6 = *(int16_t*)(buf + 26);
    int16_t f7 = *(int16_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
}
static void test48(void)
{
    char* buf = opaque(malloc(34));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 8) = 42;
    *(int16_t*)(buf + 16) = 42;
    *(int16_t*)(buf + 24) = 42;
    *(int16_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 8);
    int16_t f2 = *(int16_t*)(buf + 16);
    int16_t f3 = *(int16_t*)(buf + 24);
    int16_t f4 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test49(void)
{
    char* buf = opaque(malloc(34));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 8) = 42;
    *(int16_t*)(buf + 16) = 42;
    *(int16_t*)(buf + 24) = 42;
    *(int16_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 8);
    int16_t f2 = *(int16_t*)(buf + 16);
    int16_t f3 = *(int16_t*)(buf + 24);
    int16_t f4 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test50(void)
{
    char* buf = (malloc(34));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 8) = 42;
    *(int16_t*)(buf + 16) = 42;
    *(int16_t*)(buf + 24) = 42;
    *(int16_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 8);
    int16_t f2 = *(int16_t*)(buf + 16);
    int16_t f3 = *(int16_t*)(buf + 24);
    int16_t f4 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test51(void)
{
    char* buf = (malloc(34));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 8) = 42;
    *(int16_t*)(buf + 16) = 42;
    *(int16_t*)(buf + 24) = 42;
    *(int16_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 8);
    int16_t f2 = *(int16_t*)(buf + 16);
    int16_t f3 = *(int16_t*)(buf + 24);
    int16_t f4 = *(int16_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test52(void)
{
    char* buf = opaque(malloc(32));
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 6);
    int16_t f1 = *(int16_t*)(buf + 14);
    int16_t f2 = *(int16_t*)(buf + 22);
    int16_t f3 = *(int16_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test53(void)
{
    char* buf = opaque(malloc(32));
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 30) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 6);
    int16_t f1 = *(int16_t*)(buf + 14);
    int16_t f2 = *(int16_t*)(buf + 22);
    int16_t f3 = *(int16_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test54(void)
{
    char* buf = (malloc(32));
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 30) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 6);
    int16_t f1 = *(int16_t*)(buf + 14);
    int16_t f2 = *(int16_t*)(buf + 22);
    int16_t f3 = *(int16_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test55(void)
{
    char* buf = (malloc(32));
    *(int16_t*)(buf + 6) = 42;
    *(int16_t*)(buf + 14) = 42;
    *(int16_t*)(buf + 22) = 42;
    *(int16_t*)(buf + 30) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 6);
    int16_t f1 = *(int16_t*)(buf + 14);
    int16_t f2 = *(int16_t*)(buf + 22);
    int16_t f3 = *(int16_t*)(buf + 30);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test56(void)
{
    char* buf = opaque(malloc(26));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 12) = 42;
    *(int16_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 12);
    int16_t f2 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test57(void)
{
    char* buf = opaque(malloc(26));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 12) = 42;
    *(int16_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 12);
    int16_t f2 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test58(void)
{
    char* buf = (malloc(26));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 12) = 42;
    *(int16_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 12);
    int16_t f2 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test59(void)
{
    char* buf = (malloc(26));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 12) = 42;
    *(int16_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 12);
    int16_t f2 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test60(void)
{
    char* buf = opaque(malloc(24));
    *(int16_t*)(buf + 10) = 42;
    *(int16_t*)(buf + 22) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 10);
    int16_t f1 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test61(void)
{
    char* buf = opaque(malloc(24));
    *(int16_t*)(buf + 10) = 42;
    *(int16_t*)(buf + 22) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 10);
    int16_t f1 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test62(void)
{
    char* buf = (malloc(24));
    *(int16_t*)(buf + 10) = 42;
    *(int16_t*)(buf + 22) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 10);
    int16_t f1 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test63(void)
{
    char* buf = (malloc(24));
    *(int16_t*)(buf + 10) = 42;
    *(int16_t*)(buf + 22) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 10);
    int16_t f1 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test64(void)
{
    char* buf = opaque(malloc(20));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 18) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 18);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test65(void)
{
    char* buf = opaque(malloc(20));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 18) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 18);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test66(void)
{
    char* buf = (malloc(20));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 18) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 18);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test67(void)
{
    char* buf = (malloc(20));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 18) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 18);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test68(void)
{
    char* buf = opaque(malloc(18));
    *(int16_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test69(void)
{
    char* buf = opaque(malloc(18));
    *(int16_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test70(void)
{
    char* buf = (malloc(18));
    *(int16_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test71(void)
{
    char* buf = (malloc(18));
    *(int16_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test72(void)
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
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 4);
    int32_t f2 = *(int32_t*)(buf + 8);
    int32_t f3 = *(int32_t*)(buf + 12);
    int32_t f4 = *(int32_t*)(buf + 16);
    int32_t f5 = *(int32_t*)(buf + 20);
    int32_t f6 = *(int32_t*)(buf + 24);
    int32_t f7 = *(int32_t*)(buf + 28);
    int32_t f8 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
}
static void test73(void)
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
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 4);
    int32_t f2 = *(int32_t*)(buf + 8);
    int32_t f3 = *(int32_t*)(buf + 12);
    int32_t f4 = *(int32_t*)(buf + 16);
    int32_t f5 = *(int32_t*)(buf + 20);
    int32_t f6 = *(int32_t*)(buf + 24);
    int32_t f7 = *(int32_t*)(buf + 28);
    int32_t f8 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
}
static void test74(void)
{
    char* buf = (malloc(36));
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
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 4);
    int32_t f2 = *(int32_t*)(buf + 8);
    int32_t f3 = *(int32_t*)(buf + 12);
    int32_t f4 = *(int32_t*)(buf + 16);
    int32_t f5 = *(int32_t*)(buf + 20);
    int32_t f6 = *(int32_t*)(buf + 24);
    int32_t f7 = *(int32_t*)(buf + 28);
    int32_t f8 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
}
static void test75(void)
{
    char* buf = (malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 4) = 42;
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 24) = 42;
    *(int32_t*)(buf + 28) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 4);
    int32_t f2 = *(int32_t*)(buf + 8);
    int32_t f3 = *(int32_t*)(buf + 12);
    int32_t f4 = *(int32_t*)(buf + 16);
    int32_t f5 = *(int32_t*)(buf + 20);
    int32_t f6 = *(int32_t*)(buf + 24);
    int32_t f7 = *(int32_t*)(buf + 28);
    int32_t f8 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
    ZASSERT(f5 == 42);
    ZASSERT(f6 == 42);
    ZASSERT(f7 == 42);
    ZASSERT(f8 == 42);
}
static void test76(void)
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 24) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 8);
    int32_t f2 = *(int32_t*)(buf + 16);
    int32_t f3 = *(int32_t*)(buf + 24);
    int32_t f4 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test77(void)
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 24) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 8);
    int32_t f2 = *(int32_t*)(buf + 16);
    int32_t f3 = *(int32_t*)(buf + 24);
    int32_t f4 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test78(void)
{
    char* buf = (malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 24) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 8);
    int32_t f2 = *(int32_t*)(buf + 16);
    int32_t f3 = *(int32_t*)(buf + 24);
    int32_t f4 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test79(void)
{
    char* buf = (malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 24) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 8);
    int32_t f2 = *(int32_t*)(buf + 16);
    int32_t f3 = *(int32_t*)(buf + 24);
    int32_t f4 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test80(void)
{
    char* buf = opaque(malloc(32));
    *(int32_t*)(buf + 4) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 4);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 20);
    int32_t f3 = *(int32_t*)(buf + 28);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test81(void)
{
    char* buf = opaque(malloc(32));
    *(int32_t*)(buf + 4) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 4);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 20);
    int32_t f3 = *(int32_t*)(buf + 28);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test82(void)
{
    char* buf = (malloc(32));
    *(int32_t*)(buf + 4) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 4);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 20);
    int32_t f3 = *(int32_t*)(buf + 28);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test83(void)
{
    char* buf = (malloc(32));
    *(int32_t*)(buf + 4) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 4);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 20);
    int32_t f3 = *(int32_t*)(buf + 28);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test84(void)
{
    char* buf = opaque(malloc(28));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test85(void)
{
    char* buf = opaque(malloc(28));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test86(void)
{
    char* buf = (malloc(28));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test87(void)
{
    char* buf = (malloc(28));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 12);
    int32_t f2 = *(int32_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test88(void)
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 8);
    int32_t f1 = *(int32_t*)(buf + 20);
    int32_t f2 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test89(void)
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 8);
    int32_t f1 = *(int32_t*)(buf + 20);
    int32_t f2 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test90(void)
{
    char* buf = (malloc(36));
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 8);
    int32_t f1 = *(int32_t*)(buf + 20);
    int32_t f2 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test91(void)
{
    char* buf = (malloc(36));
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 8);
    int32_t f1 = *(int32_t*)(buf + 20);
    int32_t f2 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test92(void)
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 16);
    int32_t f2 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test93(void)
{
    char* buf = opaque(malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 16);
    int32_t f2 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test94(void)
{
    char* buf = (malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 16);
    int32_t f2 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test95(void)
{
    char* buf = (malloc(36));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 16) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 16);
    int32_t f2 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test96(void)
{
    char* buf = opaque(malloc(32));
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 12);
    int32_t f1 = *(int32_t*)(buf + 28);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test97(void)
{
    char* buf = opaque(malloc(32));
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 12);
    int32_t f1 = *(int32_t*)(buf + 28);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test98(void)
{
    char* buf = (malloc(32));
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 12);
    int32_t f1 = *(int32_t*)(buf + 28);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test99(void)
{
    char* buf = (malloc(32));
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 12);
    int32_t f1 = *(int32_t*)(buf + 28);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test100(void)
{
    char* buf = opaque(malloc(24));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 20) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 20);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test101(void)
{
    char* buf = opaque(malloc(24));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 20) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 20);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test102(void)
{
    char* buf = (malloc(24));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 20) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 20);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test103(void)
{
    char* buf = (malloc(24));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 20) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 20);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test104(void)
{
    char* buf = opaque(malloc(20));
    *(int32_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test105(void)
{
    char* buf = opaque(malloc(20));
    *(int32_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test106(void)
{
    char* buf = (malloc(20));
    *(int32_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test107(void)
{
    char* buf = (malloc(20));
    *(int32_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test108(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    int64_t f4 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test109(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    int64_t f4 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test110(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    int64_t f4 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test111(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    int64_t f4 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(f4 == 42);
}
static void test112(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test113(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test114(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test115(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test116(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test117(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test118(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test119(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test120(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test121(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test122(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test123(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test124(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test125(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test126(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test127(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test128(void)
{
    char* buf = opaque(malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 16) = 42;
    *(__int128*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 0);
    __int128 f1 = *(__int128*)(buf + 16);
    __int128 f2 = *(__int128*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test129(void)
{
    char* buf = opaque(malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 16) = 42;
    *(__int128*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 0);
    __int128 f1 = *(__int128*)(buf + 16);
    __int128 f2 = *(__int128*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test130(void)
{
    char* buf = (malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 16) = 42;
    *(__int128*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 0);
    __int128 f1 = *(__int128*)(buf + 16);
    __int128 f2 = *(__int128*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test131(void)
{
    char* buf = (malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 16) = 42;
    *(__int128*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 0);
    __int128 f1 = *(__int128*)(buf + 16);
    __int128 f2 = *(__int128*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test132(void)
{
    char* buf = opaque(malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 0);
    __int128 f1 = *(__int128*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test133(void)
{
    char* buf = opaque(malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 0);
    __int128 f1 = *(__int128*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test134(void)
{
    char* buf = (malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 0);
    __int128 f1 = *(__int128*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test135(void)
{
    char* buf = (malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 0);
    __int128 f1 = *(__int128*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test136(void)
{
    char* buf = opaque(malloc(32));
    *(__int128*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test137(void)
{
    char* buf = opaque(malloc(32));
    *(__int128*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test138(void)
{
    char* buf = (malloc(32));
    *(__int128*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test139(void)
{
    char* buf = (malloc(32));
    *(__int128*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test140(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test141(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test142(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test143(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test144(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test145(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test146(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test147(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test148(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test149(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test150(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test151(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test152(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test153(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test154(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test155(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test156(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test157(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test158(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test159(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test160(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test161(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test162(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test163(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test164(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test165(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test166(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test167(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test168(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test169(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test170(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test171(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test172(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test173(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test174(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test175(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test176(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test177(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test178(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test179(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test180(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test181(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test182(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test183(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test184(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test185(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test186(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test187(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test188(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test189(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test190(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test191(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test192(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test193(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test194(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test195(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test196(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test197(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test198(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test199(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test200(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test201(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test202(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test203(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test204(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test205(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test206(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test207(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test208(void)
{
    char* buf = opaque(malloc(16));
    *(int64_t*)(buf + 8) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test209(void)
{
    char* buf = opaque(malloc(16));
    *(int64_t*)(buf + 8) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test210(void)
{
    char* buf = (malloc(16));
    *(int64_t*)(buf + 8) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test211(void)
{
    char* buf = (malloc(16));
    *(int64_t*)(buf + 8) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test212(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test213(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test214(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test215(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test216(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test217(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test218(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test219(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test220(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test221(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test222(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test223(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test224(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test225(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test226(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test227(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test228(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test229(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test230(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test231(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test232(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test233(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test234(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test235(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test236(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test237(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test238(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test239(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test240(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test241(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test242(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test243(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test244(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test245(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test246(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test247(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test248(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test249(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test250(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test251(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test252(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test253(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test254(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test255(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test256(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test257(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test258(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test259(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test260(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test261(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test262(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test263(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test264(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test265(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test266(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test267(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test268(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test269(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test270(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test271(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test272(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test273(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test274(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test275(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test276(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test277(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test278(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test279(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test280(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test281(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test282(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test283(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test284(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test285(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test286(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test287(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test288(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test289(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test290(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test291(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
}
static void test292(void)
{
    char* buf = opaque(malloc(8));
    *(int64_t*)(buf + 0) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test293(void)
{
    char* buf = opaque(malloc(8));
    *(int64_t*)(buf + 0) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test294(void)
{
    char* buf = (malloc(8));
    *(int64_t*)(buf + 0) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test295(void)
{
    char* buf = (malloc(8));
    *(int64_t*)(buf + 0) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test296(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test297(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test298(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test299(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test300(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test301(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test302(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test303(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test304(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test305(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test306(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test307(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test308(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test309(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test310(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test311(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test312(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test313(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test314(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test315(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test316(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test317(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test318(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test319(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test320(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test321(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test322(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test323(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test324(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test325(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test326(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test327(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test328(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test329(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test330(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test331(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test332(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test333(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test334(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test335(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test336(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test337(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test338(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test339(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test340(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test341(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test342(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test343(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test344(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test345(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test346(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test347(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test348(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test349(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test350(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test351(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test352(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test353(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test354(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
}
static void test355(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test356(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test357(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test358(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test359(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test360(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test361(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test362(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test363(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test364(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test365(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test366(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test367(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test368(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test369(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test370(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test371(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
}
static void test372(void)
{
    char* buf = opaque(malloc(16));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test373(void)
{
    char* buf = opaque(malloc(16));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test374(void)
{
    char* buf = (malloc(16));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test375(void)
{
    char* buf = (malloc(16));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test376(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test377(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test378(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test379(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test380(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test381(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test382(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test383(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test384(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test385(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test386(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test387(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test388(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test389(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test390(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test391(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test392(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test393(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test394(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test395(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test396(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test397(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test398(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test399(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test400(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test401(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test402(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test403(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test404(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test405(void)
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test406(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test407(void)
{
    char* buf = (malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test408(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test409(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test410(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test411(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test412(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test413(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test414(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test415(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test416(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test417(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test418(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test419(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test420(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test421(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test422(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test423(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test424(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    char* f4 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(!strcmp(f4, "hello"));
}
static void test425(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test426(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    char* f4 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(!strcmp(f4, "hello"));
}
static void test427(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test428(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    char* f4 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(!strcmp(f4, "hello"));
}
static void test429(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test430(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    char* f4 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
    ZASSERT(!strcmp(f4, "hello"));
}
static void test431(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test432(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test433(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test434(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test435(void)
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test436(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test437(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test438(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test439(void)
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test440(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    ZASSERT(f3 == 42);
}
static void test441(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test442(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    ZASSERT(f3 == 42);
}
static void test443(void)
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test444(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    ZASSERT(f3 == 42);
}
static void test445(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test446(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    ZASSERT(f3 == 42);
}
static void test447(void)
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test448(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    ZASSERT(!strcmp(f3, "hello"));
}
static void test449(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test450(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    ZASSERT(!strcmp(f3, "hello"));
}
static void test451(void)
{
    char* buf = opaque(malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test452(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    ZASSERT(!strcmp(f3, "hello"));
}
static void test453(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test454(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    char* f2 = *(char**)(buf + 16);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    ZASSERT(!strcmp(f3, "hello"));
}
static void test455(void)
{
    char* buf = (malloc(48));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 8) = 42;
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 8);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test456(void)
{
    char* buf = opaque(malloc(16));
    *(char**)(buf + 0) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test457(void)
{
    char* buf = opaque(malloc(16));
    *(char**)(buf + 0) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test458(void)
{
    char* buf = (malloc(16));
    *(char**)(buf + 0) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test459(void)
{
    char* buf = (malloc(16));
    *(char**)(buf + 0) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    ZASSERT(!strcmp(f0, "hello"));
}
static void test460(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test461(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test462(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test463(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test464(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test465(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test466(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test467(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test468(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test469(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test470(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test471(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test472(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test473(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test474(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test475(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test476(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test477(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test478(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test479(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test480(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test481(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test482(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test483(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test484(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test485(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test486(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test487(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test488(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test489(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test490(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test491(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test492(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test493(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test494(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 24);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test495(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
}
static void test496(void)
{
    char* buf = opaque(malloc(24));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test497(void)
{
    char* buf = opaque(malloc(24));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test498(void)
{
    char* buf = opaque(malloc(24));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test499(void)
{
    char* buf = opaque(malloc(24));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test500(void)
{
    char* buf = (malloc(24));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test501(void)
{
    char* buf = (malloc(24));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test502(void)
{
    char* buf = (malloc(24));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
}
static void test503(void)
{
    char* buf = (malloc(24));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test504(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test505(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test506(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test507(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test508(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test509(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test510(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test511(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test512(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test513(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test514(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test515(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test516(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test517(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test518(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
}
static void test519(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
}
static void test520(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test521(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test522(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test523(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test524(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test525(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test526(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test527(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test528(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test529(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test530(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test531(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test532(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test533(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test534(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    int64_t f3 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(f3 == 42);
}
static void test535(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
}
static void test536(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test537(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test538(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test539(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test540(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test541(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test542(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 24);
    char* f3 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    ZASSERT(!strcmp(f3, "hello"));
}
static void test543(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
}
static void test544(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test545(void)
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test546(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test547(void)
{
    char* buf = (malloc(32));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
}
static void test548(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test549(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test550(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test551(void)
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test552(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test553(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test554(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
}
static void test555(void)
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 32);
    ZASSERT(f0 == 42);
}
static void test556(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test557(void)
{
    char* buf = opaque(malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test558(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)opaque(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
}
static void test559(void)
{
    char* buf = (malloc(48));
    *(char**)(buf + 0) = "hello";
    *(char**)(buf + 16) = "hello";
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    char* f2 = *(char**)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(!strcmp(f2, "hello"));
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
    test140();
    test141();
    test142();
    test143();
    test144();
    test145();
    test146();
    test147();
    test148();
    test149();
    test150();
    test151();
    test152();
    test153();
    test154();
    test155();
    test156();
    test157();
    test158();
    test159();
    test160();
    test161();
    test162();
    test163();
    test164();
    test165();
    test166();
    test167();
    test168();
    test169();
    test170();
    test171();
    test172();
    test173();
    test174();
    test175();
    test176();
    test177();
    test178();
    test179();
    test180();
    test181();
    test182();
    test183();
    test184();
    test185();
    test186();
    test187();
    test188();
    test189();
    test190();
    test191();
    test192();
    test193();
    test194();
    test195();
    test196();
    test197();
    test198();
    test199();
    test200();
    test201();
    test202();
    test203();
    test204();
    test205();
    test206();
    test207();
    test208();
    test209();
    test210();
    test211();
    test212();
    test213();
    test214();
    test215();
    test216();
    test217();
    test218();
    test219();
    test220();
    test221();
    test222();
    test223();
    test224();
    test225();
    test226();
    test227();
    test228();
    test229();
    test230();
    test231();
    test232();
    test233();
    test234();
    test235();
    test236();
    test237();
    test238();
    test239();
    test240();
    test241();
    test242();
    test243();
    test244();
    test245();
    test246();
    test247();
    test248();
    test249();
    test250();
    test251();
    test252();
    test253();
    test254();
    test255();
    test256();
    test257();
    test258();
    test259();
    test260();
    test261();
    test262();
    test263();
    test264();
    test265();
    test266();
    test267();
    test268();
    test269();
    test270();
    test271();
    test272();
    test273();
    test274();
    test275();
    test276();
    test277();
    test278();
    test279();
    test280();
    test281();
    test282();
    test283();
    test284();
    test285();
    test286();
    test287();
    test288();
    test289();
    test290();
    test291();
    test292();
    test293();
    test294();
    test295();
    test296();
    test297();
    test298();
    test299();
    test300();
    test301();
    test302();
    test303();
    test304();
    test305();
    test306();
    test307();
    test308();
    test309();
    test310();
    test311();
    test312();
    test313();
    test314();
    test315();
    test316();
    test317();
    test318();
    test319();
    test320();
    test321();
    test322();
    test323();
    test324();
    test325();
    test326();
    test327();
    test328();
    test329();
    test330();
    test331();
    test332();
    test333();
    test334();
    test335();
    test336();
    test337();
    test338();
    test339();
    test340();
    test341();
    test342();
    test343();
    test344();
    test345();
    test346();
    test347();
    test348();
    test349();
    test350();
    test351();
    test352();
    test353();
    test354();
    test355();
    test356();
    test357();
    test358();
    test359();
    test360();
    test361();
    test362();
    test363();
    test364();
    test365();
    test366();
    test367();
    test368();
    test369();
    test370();
    test371();
    test372();
    test373();
    test374();
    test375();
    test376();
    test377();
    test378();
    test379();
    test380();
    test381();
    test382();
    test383();
    test384();
    test385();
    test386();
    test387();
    test388();
    test389();
    test390();
    test391();
    test392();
    test393();
    test394();
    test395();
    test396();
    test397();
    test398();
    test399();
    test400();
    test401();
    test402();
    test403();
    test404();
    test405();
    test406();
    test407();
    test408();
    test409();
    test410();
    test411();
    test412();
    test413();
    test414();
    test415();
    test416();
    test417();
    test418();
    test419();
    test420();
    test421();
    test422();
    test423();
    test424();
    test425();
    test426();
    test427();
    test428();
    test429();
    test430();
    test431();
    test432();
    test433();
    test434();
    test435();
    test436();
    test437();
    test438();
    test439();
    test440();
    test441();
    test442();
    test443();
    test444();
    test445();
    test446();
    test447();
    test448();
    test449();
    test450();
    test451();
    test452();
    test453();
    test454();
    test455();
    test456();
    test457();
    test458();
    test459();
    test460();
    test461();
    test462();
    test463();
    test464();
    test465();
    test466();
    test467();
    test468();
    test469();
    test470();
    test471();
    test472();
    test473();
    test474();
    test475();
    test476();
    test477();
    test478();
    test479();
    test480();
    test481();
    test482();
    test483();
    test484();
    test485();
    test486();
    test487();
    test488();
    test489();
    test490();
    test491();
    test492();
    test493();
    test494();
    test495();
    test496();
    test497();
    test498();
    test499();
    test500();
    test501();
    test502();
    test503();
    test504();
    test505();
    test506();
    test507();
    test508();
    test509();
    test510();
    test511();
    test512();
    test513();
    test514();
    test515();
    test516();
    test517();
    test518();
    test519();
    test520();
    test521();
    test522();
    test523();
    test524();
    test525();
    test526();
    test527();
    test528();
    test529();
    test530();
    test531();
    test532();
    test533();
    test534();
    test535();
    test536();
    test537();
    test538();
    test539();
    test540();
    test541();
    test542();
    test543();
    test544();
    test545();
    test546();
    test547();
    test548();
    test549();
    test550();
    test551();
    test552();
    test553();
    test554();
    test555();
    test556();
    test557();
    test558();
    test559();
    return 0;
}
