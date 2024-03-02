#include <stdfil.h>

_Bool zinbounds(void* ptr)
{
    return ptr >= zgetlower(ptr) && ptr < zgetupper(ptr);
}

_Bool zvalinbounds(void* ptr, __SIZE_TYPE__ size)
{
    if (!size)
        return 1;
    return zinbounds(ptr) && zinbounds((char*)ptr + size - 1);
}

_Bool zisptr(void* ptr)
{
    return zptrphase(ptr) != -1;
}

_Bool zisintorptr(void* ptr)
{
    return zisint(ptr) || zisptr(ptr);
}

void zmemmove_nullify(void* dst_ptr, const void* src_ptr, __SIZE_TYPE__ count)
{
    char* dst = (char*)dst_ptr;
    char* src = (char*)src_ptr;

    __SIZE_TYPE__ index;

    if (dst < src) {
        for (index = 0; index < count; ++index) {
            if (zisint(dst + index)) {
                if (zisint(src + index)) {
                    dst[index] = src[index];
                    continue;
                }
                ZASSERT(zptrphase(src + index) != -1);
                dst[index] = 0;
                continue;
            }
            ZASSERT(!zptrphase(dst + index));
            if (!zptrphase(src + index) && count - index >= sizeof(void*)) {
                *(void**)(dst + index) = *(void**)(src + index);
                index += sizeof(void*) - 1;
                continue;
            }
            *(void**)(dst + index) = (void*)0;
            __SIZE_TYPE__ index_in_ptr;
            for (index_in_ptr = sizeof(void*); index_in_ptr--;)
                ZASSERT(index + index_in_ptr >= count || zisintorptr(src + index + index_in_ptr));
            index += sizeof(void*) - 1;
        }
    } else {
        for (index = count; index--;) {
            if (zisint(dst + index)) {
                if (zisint(src + index)) {
                    dst[index] = src[index];
                    continue;
                }
                ZASSERT(zptrphase(src + index) != -1);
                dst[index] = 0;
                continue;
            }
            ZASSERT(zptrphase(dst + index) == sizeof(void*) - 1);
            if (zptrphase(src + index) == sizeof(void*) - 1 && index >= sizeof(void*) - 1) {
                *(void**)(dst + index - sizeof(void*) + 1) = *(void**)(src + index - sizeof(void*) + 1);
                index -= sizeof(void*) - 1;
                continue;
            }
            *(void**)(dst + index - sizeof(void*) + 1) = (void*)0;
            __SIZE_TYPE__ index_in_ptr;
            for (index_in_ptr = sizeof(void*); index_in_ptr--;)
                ZASSERT(index_in_ptr > index || zisintorptr(src + index - index_in_ptr));
            if (index < sizeof(void*) - 1)
                index = 0;
            else
                index -= sizeof(void*) - 1;
        }
    }
}

void* zalloc_like(void* obj)
{
    /* FIXME: This doesn't work right for flexes, but probably only because zalloc_with_type doesn't
       work for flexes. */
    void* result = zalloc_with_type(zgettype(obj), (char*)zgetupper(obj) - (char*)zgetlower(obj));
    ZASSERT(result);
    return (char*)result + ((char*)obj - (char*)zgetlower(obj));
}

typedef struct {
    const int* address;
    int expected_value;
} compare_and_park_data;

static _Bool compare_and_park_condition(void* arg)
{
    compare_and_park_data* data = (compare_and_park_data*)arg;
    return *data->address == data->expected_value;
}

static void empty_before_sleep(void* arg)
{
    (void)arg;
}

_Bool zcompare_and_park(const int* address, int expected_value, double absolute_timeout_in_milliseconds)
{
    compare_and_park_data data;
    data.address = (const int*)((const char*)address - ((unsigned long)address % sizeof(int)));
    data.expected_value = expected_value;
    return zpark_if(address, compare_and_park_condition, empty_before_sleep, &data,
                    absolute_timeout_in_milliseconds);
}

unsigned zthread_self_id(void)
{
    return zthread_get_id(zthread_self());
}

void* zthread_self_cookie(void)
{
    return zthread_get_cookie(zthread_self());
}
