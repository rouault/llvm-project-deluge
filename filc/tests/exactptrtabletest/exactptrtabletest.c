#include <stdfil.h>

static void doit(void)
{
    int* object1 = zgc_alloc(sizeof(int));
    int* object2 = zgc_alloc(sizeof(int));
    ZASSERT(zhasvalidcap(object1));
    ZASSERT(zhasvalidcap(object2));
    zexact_ptrtable* table = zexact_ptrtable_new();
    ZASSERT(!zexact_ptrtable_encode(table, 0));
    ZASSERT(!zexact_ptrtable_decode(table, 0));
    ZASSERT(zexact_ptrtable_encode(table, (void*)666) == 666);
    ZASSERT(zexact_ptrtable_decode(table, 666) == (void*)666);
    unsigned long index1 = zexact_ptrtable_encode(table, object1);
    unsigned long index2 = zexact_ptrtable_encode(table, object2);
    ZASSERT((int*)index1 == object1);
    ZASSERT((int*)index2 == object2);
    ZASSERT(index1 != index2);
    ZASSERT(zexact_ptrtable_encode(table, object1) == index1);
    ZASSERT(zexact_ptrtable_encode(table, object2) == index2);
    int* object1_2 = zexact_ptrtable_decode(table, index1);
    int* object2_2 = zexact_ptrtable_decode(table, index2);
    ZASSERT(object1_2 == object1);
    ZASSERT(object2_2 == object2);
    *object1_2 = 42;
    ZASSERT(*object1 == 42);
    *object2_2 = 666;
    ZASSERT(*object2 == 666);
    ZASSERT(zhasvalidcap(object1));
    ZASSERT(zhasvalidcap(object2));
    ZASSERT(zhasvalidcap(object1_2));
    ZASSERT(zhasvalidcap(object2_2));
    zgc_free(object1);
    ZASSERT(!zhasvalidcap(object1));
    ZASSERT(zexact_ptrtable_encode(table, object1) == index1);
    object1_2 = zexact_ptrtable_decode(table, index1);
    ZASSERT(object1_2 == object1);
    ZASSERT(!zhasvalidcap(object1_2));
    zgc_request_and_wait();
    object1 = zgc_alloc(sizeof(int));
    unsigned long index1_2 = zexact_ptrtable_encode(table, object1);
    ZASSERT((int*)index1_2 == object1);
    ZASSERT(zexact_ptrtable_decode(table, index1_2) == object1);
}

int main()
{
    doit();
    zprintf("Success\n");
    return 0;
}

