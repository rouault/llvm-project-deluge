/*
 * Copyright (c) 2024 Epic Games, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY EPIC GAMES, INC. ``AS IS AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL EPIC GAMES, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include <stdfil.h>
#include <pizlonated_syscalls.h>
#include <pizlonated_runtime.h>

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

void* zmkptr(void* object, unsigned long address)
{
    char* ptr = (char*)object;
    ptr -= (unsigned long)object;
    ptr += address;
    return ptr;
}

void* zorptr(void* ptr, unsigned long bits)
{
    return zmkptr(ptr, (unsigned long)ptr | bits);
}

void* zandptr(void* ptr, unsigned long bits)
{
    return zmkptr(ptr, (unsigned long)ptr & bits);
}

void* zxorptr(void* ptr, unsigned long bits)
{
    return zmkptr(ptr, (unsigned long)ptr ^ bits);
}

void* zretagptr(void* newptr, void* oldptr, unsigned long mask)
{
    ZASSERT(!((unsigned long)newptr & ~mask));
    return zorptr(newptr, (unsigned long)oldptr & ~mask);
}

static void copy_byte_to_nonptr(char* dst, char* src, __SIZE_TYPE__ index)
{
    if (zisint(src + index)) {
        char value = src[index];
        if (!value && zisunset(dst + index))
            return;
        dst[index] = value;
        return;
    }
    if (zisunset(dst + index))
        return;
    dst[index] = 0;
}

void zmemmove_nullify(void* dst_ptr, const void* src_ptr, __SIZE_TYPE__ count)
{
    char* dst = (char*)dst_ptr;
    char* src = (char*)src_ptr;

    __SIZE_TYPE__ index;

    if (dst < src) {
        for (index = 0; index < count; ++index) {
            int dst_ptrphase = zptrphase(dst + index);
            if (dst_ptrphase < 0) {
                copy_byte_to_nonptr(dst, src, index);
                continue;
            }
            ZASSERT(!dst_ptrphase);
            *(void**)(dst + index) = (void*)0;
            index += sizeof(void*) - 1;
        }
    } else {
        for (index = count; index--;) {
            int dst_ptrphase = zptrphase(dst + index);
            if (dst_ptrphase < 0) {
                copy_byte_to_nonptr(dst, src, index);
                continue;
            }
            ZASSERT(dst_ptrphase == sizeof(void*) - 1);
            *(void**)(dst + index - sizeof(void*) + 1) = (void*)0;
            if (index < sizeof(void*) - 1)
                index = 0;
            else
                index -= sizeof(void*) - 1;
        }
    }
}

_Bool zunfenced_intense_cas_ptr(void** ptr, void** expected_ptr, void* new_value)
{
    void* expected = *expected_ptr;
    void* result = zunfenced_strong_cas_ptr(ptr, expected, new_value);
    *expected_ptr = result;
    return result == expected;
}

_Bool zintense_cas_ptr(void** ptr, void** expected_ptr, void* new_value)
{
    void* expected = *expected_ptr;
    void* result = zstrong_cas_ptr(ptr, expected, new_value);
    *expected_ptr = result;
    return result == expected;
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

zpark_result zcompare_and_park(
    const int* address, int expected_value, double absolute_timeout_in_milliseconds)
{
    compare_and_park_data data;
    data.address = (const int*)((const char*)address - ((unsigned long)address % sizeof(int)));
    data.expected_value = expected_value;
    return zpark_if(address, compare_and_park_condition, empty_before_sleep, &data,
                    absolute_timeout_in_milliseconds);
}

void* zthread_self_cookie(void)
{
    return zthread_get_cookie(zthread_self());
}
