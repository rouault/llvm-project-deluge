#include <stdfil.h>
#include <stdlib.h>

#define unfenced_weak_cas_ptr(ptr, expected, new_value) ({ \
        void* tmp = expected; \
        __c11_atomic_compare_exchange_weak( \
            (void*_Atomic*)ptr, &tmp, new_value, __ATOMIC_RELAXED, __ATOMIC_RELAXED); \
    })

#define weak_cas_ptr(ptr, expected, new_value) ({ \
        void* tmp = expected; \
        __c11_atomic_compare_exchange_weak( \
            (void*_Atomic*)ptr, &tmp, new_value, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
    })

#define unfenced_strong_cas_ptr(ptr, expected, new_value) ({ \
        void* result = expected; \
        __c11_atomic_compare_exchange_strong( \
            (void*_Atomic*)ptr, &result, new_value, __ATOMIC_RELAXED, __ATOMIC_RELAXED); \
        result; \
    })

#define strong_cas_ptr(ptr, expected, new_value) ({ \
        void* result = expected; \
        __c11_atomic_compare_exchange_strong( \
            (void*_Atomic*)ptr, &result, new_value, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
        result; \
    })

#define unfenced_intense_cas_ptr(ptr, expected, new_value) \
    __c11_atomic_compare_exchange_strong( \
        (void*_Atomic*)ptr, expected, new_value, __ATOMIC_RELAXED, __ATOMIC_RELAXED)

#define intense_cas_ptr(ptr, expected, new_value) \
    __c11_atomic_compare_exchange_strong( \
        (void*_Atomic*)ptr, expected, new_value, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define unfenced_xchg_ptr(ptr, new_value) \
    __c11_atomic_exchange((void*_Atomic*)ptr, new_value, __ATOMIC_RELAXED)

#define xchg_ptr(ptr, new_value) \
    __c11_atomic_exchange((void*_Atomic*)ptr, new_value, __ATOMIC_SEQ_CST)

#define unfenced_atomic_store_ptr(ptr, new_value) \
    __c11_atomic_store((void*_Atomic*)ptr, new_value, __ATOMIC_RELAXED)

#define atomic_store_ptr(ptr, new_value) \
    __c11_atomic_store((void*_Atomic*)ptr, new_value, __ATOMIC_SEQ_CST)

#define unfenced_atomic_load_ptr(ptr) \
    __c11_atomic_load((void*_Atomic*)ptr, __ATOMIC_RELAXED)

#define atomic_load_ptr(ptr) \
    __c11_atomic_load((void*_Atomic*)ptr, __ATOMIC_SEQ_CST)

int main()
{
    void* ptr = 0;
    void* a = malloc(1);
    void* b = malloc(1);
    void* expected = 0;
    ZASSERT(a != b);
    ZASSERT(!ptr);
    ZASSERT(!unfenced_weak_cas_ptr(&ptr, a, b));
    ZASSERT(!ptr);
    while (!unfenced_weak_cas_ptr(&ptr, NULL, a));
    ZASSERT(ptr == a);
    ZASSERT(!weak_cas_ptr(&ptr, NULL, a));
    ZASSERT(ptr == a);
    while (!weak_cas_ptr(&ptr, a, b));
    ZASSERT(ptr == b);
    ZASSERT(unfenced_strong_cas_ptr(&ptr, a, b) == b);
    ZASSERT(ptr == b);
    ZASSERT(unfenced_strong_cas_ptr(&ptr, b, NULL) == b);
    ZASSERT(!ptr);
    ZASSERT(!strong_cas_ptr(&ptr, b, NULL));
    ZASSERT(!ptr);
    ZASSERT(!strong_cas_ptr(&ptr, NULL, a));
    ZASSERT(ptr == a);
    expected = NULL;
    ZASSERT(!unfenced_intense_cas_ptr(&ptr, &expected, a));
    ZASSERT(expected == a);
    ZASSERT(ptr == a);
    expected = a;
    ZASSERT(unfenced_intense_cas_ptr(&ptr, &expected, b));
    ZASSERT(expected == a);
    ZASSERT(ptr == b);
    expected = a;
    ZASSERT(!intense_cas_ptr(&ptr, &expected, b));
    ZASSERT(expected == b);
    ZASSERT(ptr = b);
    expected = b;
    ZASSERT(intense_cas_ptr(&ptr, &expected, a));
    ZASSERT(expected == b);
    ZASSERT(ptr == a);
    ZASSERT(xchg_ptr(&ptr, b) == a);
    ZASSERT(ptr == b);
    ZASSERT(unfenced_xchg_ptr(&ptr, a) == b);
    ZASSERT(ptr == a);
    atomic_store_ptr(&ptr, b);
    ZASSERT(ptr == b);
    unfenced_atomic_store_ptr(&ptr, a);
    ZASSERT(ptr == a);
    ZASSERT(atomic_load_ptr(&ptr) == a);
    ZASSERT(unfenced_atomic_load_ptr(&ptr) == a);
    return 0;
}

