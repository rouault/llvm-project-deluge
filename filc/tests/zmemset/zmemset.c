#include <stdfil.h>

static void test1(void)
{
    char* array[4];
    zmemset((char*)array + 8, 0, 23);
    unsigned i;
    for (i = 4; i--;)
        array[i] = "bruh";
}

static void test2(void)
{
    char* array[4];
    zmemset((char*)array + 15, 0, 1);
    unsigned i;
    for (i = 4; i--;)
        array[i] = "bruh";
}

static void test3(void)
{
    char* array[4];
    zmemset((char*)array + 1, 0, 14);
    unsigned i;
    for (i = 4; i--;)
        array[i] = "bruh";
}

static void test4(void)
{
    char* array[4];
    zmemset((char*)array + 1, 0, 23);
    unsigned i;
    for (i = 4; i--;)
        array[i] = "bruh";
}

static void test5(void)
{
    char* array[4];
    zmemset((char*)array + 1, 0, 31);
    unsigned i;
    for (i = 4; i--;)
        array[i] = "bruh";
}

static void test6(void)
{
    char* array[4];
    zmemset(array, 0, 1);
    unsigned i;
    for (i = 4; i--;)
        array[i] = "bruh";
}

static void test7(void)
{
    char array[64];
    zmemset(array, 42, 64);
    unsigned i;
    for (i = 64; i--;)
        ZASSERT(array[i] == 42);
    zmemset((char*)array + 16, 0, 47);
    for (i = 16; i--;)
        ZASSERT(array[i] == 42);
    for (i = 63; i-->16;)
        ZASSERT(!array[i]);
    ZASSERT(array[63] == 42);
}

static void test8(void)
{
    char array[64];
    zmemset(array, 42, 64);
    unsigned i;
    for (i = 64; i--;)
        ZASSERT(array[i] == 42);
    zmemset((char*)array + 15, 0, 1);
    for (i = 15; i--;)
        ZASSERT(array[i] == 42);
    ZASSERT(!array[15]);
    for (i = 64; i-->16;)
        ZASSERT(array[i] == 42);
}

static void test9(void)
{
    char array[64];
    zmemset(array, 42, 64);
    unsigned i;
    for (i = 64; i--;)
        ZASSERT(array[i] == 42);
    zmemset((char*)array + 1, 0, 14);
    ZASSERT(array[0] == 42);
    for (i = 15; i-->1;)
        ZASSERT(!array[i]);
    for (i = 64; i-->15;)
        ZASSERT(array[i] = 42);
}

static void test10(void)
{
    char array[64];
    zmemset(array, 42, 64);
    unsigned i;
    for (i = 64; i--;)
        ZASSERT(array[i] == 42);
    zmemset((char*)array + 1, 0, 47);
    ZASSERT(array[0] == 42);
    for (i = 48; i-->1;)
        ZASSERT(!array[i]);
    for (i = 64; i-->48;)
        ZASSERT(array[i] == 42);
}

static void test11(void)
{
    char array[64];
    zmemset(array, 42, 64);
    unsigned i;
    for (i = 64; i--;)
        ZASSERT(array[i] == 42);
    zmemset((char*)array + 1, 0, 62);
    ZASSERT(array[0] == 42);
    for (i = 63; i-->1;)
        ZASSERT(!array[i]);
    ZASSERT(array[63] == 42);
}

static void test12(void)
{
    char array[64];
    zmemset(array, 42, 64);
    unsigned i;
    for (i = 64; i--;)
        ZASSERT(array[i] == 42);
    zmemset(array, 0, 1);
    ZASSERT(!array[0]);
    for (i = 64; i-->1;)
        ZASSERT(array[i] == 42);
}

int main(void)
{
    char buf[10];
    zmemset(buf, 'f', 10);
    zmemset(buf + 9, 0, 1);
    zmemset((void*)666, 0, 0);
    zmemset((void*)666, 42, 0);
    zprint(buf);

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
    
    return 0;
}

