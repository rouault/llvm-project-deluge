#include <stdfil.h>
#include <stdlib.h>
#include <atomic>

#define unfenced_weak_cas_ptr(ptr, expected, new_value) ({ \
        void* tmp = expected; \
        reinterpret_cast<std::atomic<void*>*>(ptr)->compare_exchange_weak( \
            tmp, new_value, std::memory_order::relaxed); \
    })

#define weak_cas_ptr(ptr, expected, new_value) ({ \
        void* tmp = expected; \
        reinterpret_cast<std::atomic<void*>*>(ptr)->compare_exchange_weak(tmp, new_value); \
    })

#define unfenced_strong_cas_ptr(ptr, expected, new_value) ({ \
        void* result = expected; \
        reinterpret_cast<std::atomic<void*>*>(ptr)->compare_exchange_strong( \
            result, new_value, std::memory_order::relaxed); \
        result; \
    })

#define strong_cas_ptr(ptr, expected, new_value) ({ \
        void* result = expected; \
        reinterpret_cast<std::atomic<void*>*>(ptr)->compare_exchange_strong(result, new_value); \
        result; \
    })

#define unfenced_intense_cas_ptr(ptr, expected, new_value) \
    reinterpret_cast<std::atomic<void*>*>(ptr)->compare_exchange_strong( \
        *(expected), new_value, std::memory_order::relaxed)

#define intense_cas_ptr(ptr, expected, new_value) \
    reinterpret_cast<std::atomic<void*>*>(ptr)->compare_exchange_strong(*(expected), new_value)

#define unfenced_xchg_ptr(ptr, new_value) \
    reinterpret_cast<std::atomic<void*>*>(ptr)->exchange(new_value, std::memory_order::relaxed)

#define xchg_ptr(ptr, new_value) \
    reinterpret_cast<std::atomic<void*>*>(ptr)->exchange(new_value)

#define unfenced_atomic_store_ptr(ptr, new_value) \
    reinterpret_cast<std::atomic<void*>*>(ptr)->store(new_value, std::memory_order::relaxed)

#define atomic_store_ptr(ptr, new_value) \
    reinterpret_cast<std::atomic<void*>*>(ptr)->store(new_value)

#define unfenced_atomic_load_ptr(ptr) \
    reinterpret_cast<std::atomic<void*>*>(ptr)->load(std::memory_order::relaxed)

#define atomic_load_ptr(ptr) \
    reinterpret_cast<std::atomic<void*>*>(ptr)->load()

int main(int argc, char** argv)
{
    void* ptr = nullptr;
    void* a = malloc(1);
    void* b = malloc(1);
    void* expected = nullptr;
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

