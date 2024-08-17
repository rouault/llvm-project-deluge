#include <stdfil.h>
#include <stdlib.h>

int main()
{
    void* ptr = NULL;
    void* a = malloc(1);
    void* b = malloc(1);
    void* expected = NULL;
    ZASSERT(a != b);
    ZASSERT(!ptr);
    ZASSERT(!zunfenced_weak_cas_ptr(&ptr, a, b));
    ZASSERT(!ptr);
    while (!zunfenced_weak_cas_ptr(&ptr, NULL, a));
    ZASSERT(ptr == a);
    ZASSERT(!zweak_cas_ptr(&ptr, NULL, a));
    ZASSERT(ptr == a);
    while (!zweak_cas_ptr(&ptr, a, b));
    ZASSERT(ptr == b);
    ZASSERT(zunfenced_strong_cas_ptr(&ptr, a, b) == b);
    ZASSERT(ptr == b);
    ZASSERT(zunfenced_strong_cas_ptr(&ptr, b, NULL) == b);
    ZASSERT(!ptr);
    ZASSERT(!zstrong_cas_ptr(&ptr, b, NULL));
    ZASSERT(!ptr);
    ZASSERT(!zstrong_cas_ptr(&ptr, NULL, a));
    ZASSERT(ptr == a);
    expected = NULL;
    ZASSERT(!zunfenced_intense_cas_ptr(&ptr, &expected, a));
    ZASSERT(expected == a);
    ZASSERT(ptr == a);
    expected = a;
    ZASSERT(zunfenced_intense_cas_ptr(&ptr, &expected, b));
    ZASSERT(expected == a);
    ZASSERT(ptr == b);
    expected = a;
    ZASSERT(!zintense_cas_ptr(&ptr, &expected, b));
    ZASSERT(expected == b);
    ZASSERT(ptr = b);
    expected = b;
    ZASSERT(zintense_cas_ptr(&ptr, &expected, a));
    ZASSERT(expected == b);
    ZASSERT(ptr == a);
    ZASSERT(zxchg_ptr(&ptr, b) == a);
    ZASSERT(ptr == b);
    ZASSERT(zunfenced_xchg_ptr(&ptr, a) == b);
    ZASSERT(ptr == a);
    zatomic_store_ptr(&ptr, b);
    ZASSERT(ptr == b);
    zunfenced_atomic_store_ptr(&ptr, a);
    ZASSERT(ptr == a);
    ZASSERT(zatomic_load_ptr(&ptr) == a);
    ZASSERT(zunfenced_atomic_load_ptr(&ptr) == a);
    return 0;
}

