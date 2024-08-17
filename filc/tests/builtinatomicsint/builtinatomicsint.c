#include <stdfil.h>
#include <stdlib.h>

#define unfenced_weak_cas_int(ptr, expected, new_value) ({ \
        int tmp = expected; \
        __c11_atomic_compare_exchange_weak( \
            (_Atomic int*)ptr, &tmp, new_value, __ATOMIC_RELAXED, __ATOMIC_RELAXED); \
    })

#define weak_cas_int(ptr, expected, new_value) ({ \
        int tmp = expected; \
        __c11_atomic_compare_exchange_weak( \
            (_Atomic int*)ptr, &tmp, new_value, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
    })

#define unfenced_strong_cas_int(ptr, expected, new_value) ({ \
        int result = expected; \
        __c11_atomic_compare_exchange_strong( \
            (_Atomic int*)ptr, &result, new_value, __ATOMIC_RELAXED, __ATOMIC_RELAXED); \
        result; \
    })

#define strong_cas_int(ptr, expected, new_value) ({ \
        int result = expected; \
        __c11_atomic_compare_exchange_strong( \
            (_Atomic int*)ptr, &result, new_value, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
        result; \
    })

#define unfenced_intense_cas_int(ptr, expected, new_value) \
    __c11_atomic_compare_exchange_strong( \
        (_Atomic int*)ptr, expected, new_value, __ATOMIC_RELAXED, __ATOMIC_RELAXED)

#define intense_cas_int(ptr, expected, new_value) \
    __c11_atomic_compare_exchange_strong( \
        (_Atomic int*)ptr, expected, new_value, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define unfenced_xchg_int(ptr, new_value) \
    __c11_atomic_exchange((_Atomic int*)ptr, new_value, __ATOMIC_RELAXED)

#define xchg_int(ptr, new_value) \
    __c11_atomic_exchange((_Atomic int*)ptr, new_value, __ATOMIC_SEQ_CST)

#define unfenced_atomic_store_int(ptr, new_value) \
    __c11_atomic_store((_Atomic int*)ptr, new_value, __ATOMIC_RELAXED)

#define atomic_store_int(ptr, new_value) \
    __c11_atomic_store((_Atomic int*)ptr, new_value, __ATOMIC_SEQ_CST)

#define unfenced_atomic_load_int(ptr) \
    __c11_atomic_load((_Atomic int*)ptr, __ATOMIC_RELAXED)

#define atomic_load_int(ptr) \
    __c11_atomic_load((_Atomic int*)ptr, __ATOMIC_SEQ_CST)

int main()
{
    int value = 0;
    int a = 1;
    int b = 2;
    int expected = 0;
    ZASSERT(a != b);
    ZASSERT(!value);
    ZASSERT(!unfenced_weak_cas_int(&value, a, b));
    ZASSERT(!value);
    while (!unfenced_weak_cas_int(&value, 0, a));
    ZASSERT(value == a);
    ZASSERT(!weak_cas_int(&value, 0, a));
    ZASSERT(value == a);
    while (!weak_cas_int(&value, a, b));
    ZASSERT(value == b);
    ZASSERT(unfenced_strong_cas_int(&value, a, b) == b);
    ZASSERT(value == b);
    ZASSERT(unfenced_strong_cas_int(&value, b, 0) == b);
    ZASSERT(!value);
    ZASSERT(!strong_cas_int(&value, b, 0));
    ZASSERT(!value);
    ZASSERT(!strong_cas_int(&value, 0, a));
    ZASSERT(value == a);
    expected = 0;
    ZASSERT(!unfenced_intense_cas_int(&value, &expected, a));
    ZASSERT(expected == a);
    ZASSERT(value == a);
    expected = a;
    ZASSERT(unfenced_intense_cas_int(&value, &expected, b));
    ZASSERT(expected == a);
    ZASSERT(value == b);
    expected = a;
    ZASSERT(!intense_cas_int(&value, &expected, b));
    ZASSERT(expected == b);
    ZASSERT(value = b);
    expected = b;
    ZASSERT(intense_cas_int(&value, &expected, a));
    ZASSERT(expected == b);
    ZASSERT(value == a);
    ZASSERT(xchg_int(&value, b) == a);
    ZASSERT(value == b);
    ZASSERT(unfenced_xchg_int(&value, a) == b);
    ZASSERT(value == a);
    atomic_store_int(&value, b);
    ZASSERT(value == b);
    unfenced_atomic_store_int(&value, a);
    ZASSERT(value == a);
    ZASSERT(atomic_load_int(&value) == a);
    ZASSERT(unfenced_atomic_load_int(&value) == a);
    return 0;
}

