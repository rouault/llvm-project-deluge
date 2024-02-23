/*
 * Copyright (C) 2015-2016 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

/* I rewrote this code from C++ to C and removed the fairness hack and WordLock. */

#include "pas_config.h"

#if LIBPAS_ENABLED

#include "filc_parking_lot.h"
#include "filc_runtime.h"
#include <stdlib.h>

static const bool verbose = false;

struct bucket;
struct hashtable;
struct ptr_array;
struct thread_data;
typedef struct bucket bucket;
typedef struct hashtable hashtable;
typedef struct ptr_array ptr_array;
typedef struct thread_data thread_data;

struct ptr_array {
    void** array;
    unsigned size;
    unsigned capacity;
};

struct thread_data {
    unsigned ref_count;
    pas_system_mutex lock;
    pas_system_condition condition;
    const void* address;
    thread_data* next_in_queue;
};

enum dequeue_result {
    dequeue_ignore,
    dequeue_remove_and_continue,
    dequeue_remove_and_stop
};

typedef enum dequeue_result dequeue_result;

struct bucket {
    thread_data* queue_head;
    thread_data* queue_tail;
    pas_lock lock;
};

struct hashtable {
    unsigned size;
    bucket* data[1];
};

static void ptr_array_construct(ptr_array* array)
{
    static const unsigned initial_capacity = 5;
    
    array->array = filc_allocate_utility(sizeof(void*) * initial_capacity);
    array->size = 0;
    array->capacity = initial_capacity;
}

static void ptr_array_destruct(ptr_array* array)
{
    filc_deallocate(array->array);
}

static void ptr_array_add(ptr_array* array, void* ptr)
{
    if (array->size >= array->capacity) {
        void** new_array;
        unsigned new_capacity;
        
        PAS_ASSERT(array->size == array->capacity);
        PAS_ASSERT(!pas_mul_uint32_overflow(array->capacity, 2, &new_capacity));

        new_array = filc_allocate_utility(sizeof(void*) * new_capacity);
        memcpy(new_array, array->array, sizeof(void*) * array->size);

        filc_deallocate(array->array);
        array->array = new_array;
        array->capacity = new_capacity;
        PAS_ASSERT(array->size < array->capacity);
    }

    array->array[array->size++] = ptr;
}

static void bucket_construct(bucket* bucket)
{
    bucket->queue_head = NULL;
    bucket->queue_tail = NULL;
    pas_lock_construct(&bucket->lock);
}

static bucket* bucket_create(void)
{
    bucket* result = (bucket*)filc_allocate_utility(sizeof(bucket));
    if (verbose) {
        pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": creating bucket at %p\n",
                pas_get_current_system_thread_id(), result);
    }
    bucket_construct(result);
    return result;
}

static void bucket_destroy(bucket* bucket)
{
    filc_deallocate(bucket);
}

static void bucket_enqueue(bucket* bucket, thread_data* data)
{
    if (verbose) {
        pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": enqueueing %p with address = %p onto %p\n",
                pas_get_current_system_thread_id(), data, data->address, bucket);
    }

    if (bucket->queue_tail) {
        bucket->queue_tail->next_in_queue = data;
        bucket->queue_tail = data;
        return;
    }

    bucket->queue_head = data;
    bucket->queue_tail = data;
}

static void bucket_generic_dequeue(
    bucket* bucket, dequeue_result (*callback)(thread_data* data, void* arg), void* arg)
{
    if (verbose) {
        pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": dequeueing from bucket at %p\n",
                pas_get_current_system_thread_id(), bucket);
    }

    if (!bucket->queue_head)
        return;

    bool should_continue = true;
    thread_data** current_ptr = &bucket->queue_head;
    thread_data* previous = NULL;
    bool did_dequeue = false;

    while (should_continue) {
        thread_data* current = *current_ptr;
        if (verbose) {
            pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": got thread %p\n",
                    pas_get_current_system_thread_id(), current);
        }
        if (!current)
            break;
        dequeue_result result = callback(current, arg);
        switch (result) {
        case dequeue_ignore:
            previous = current;
            current_ptr = &(*current_ptr)->next_in_queue;
            break;
        case dequeue_remove_and_stop:
            should_continue = false;
        case dequeue_remove_and_continue:
            if (current == bucket->queue_tail)
                bucket->queue_tail = previous;
            did_dequeue = true;
            *current_ptr = current->next_in_queue;
            current->next_in_queue = NULL;
            break;
        }
    }

    PAS_ASSERT(!!bucket->queue_head == !!bucket->queue_tail);
}

static dequeue_result bucket_dequeue_callback(thread_data* data, void* arg)
{
    thread_data** result;
    result = (thread_data**)arg;
    *result = data;
    return dequeue_remove_and_stop;
}

static thread_data* bucket_dequeue(bucket* bucket)
{
    thread_data* result;
    bucket_generic_dequeue(bucket, bucket_dequeue_callback, &result);
    return result;
}

static hashtable* hashtable_create(unsigned size)
{
    size_t total_size;
    
    PAS_ASSERT(size >= 1);
    PAS_ASSERT(!pas_mul_uintptr_overflow(size, sizeof(bucket*), &total_size));
    PAS_ASSERT(!pas_add_uintptr_overflow(total_size, PAS_OFFSETOF(hashtable, data), &total_size));

    hashtable* result = (hashtable*)filc_allocate_utility(total_size);
    result->size = size;
    pas_zero_memory(result->data, sizeof(bucket*) * size);

    return result;
}

static void hashtable_destroy(hashtable* table)
{
    filc_deallocate(table);
}

static hashtable* the_table;
static unsigned num_threads;

static const unsigned max_load_factor = 3;
static const unsigned growth_factor = 2;

static hashtable* ensure_hashtable(void)
{
    for (;;) {
        hashtable* current_table = the_table;

        if (current_table)
            return current_table;

        current_table = hashtable_create(max_load_factor);
        if (pas_compare_and_swap_ptr_weak(&the_table, NULL, current_table))
            return current_table;

        hashtable_destroy(current_table);
    }
}

static int compare_ptr(const void* a_ptr, const void* b_ptr)
{
    void* a = *(void**)a_ptr;
    void* b = *(void**)b_ptr;
    if (a < b)
        return -1;
    if (a == b)
        return 0;
    return 1;
}

static void lock_hashtable(ptr_array* buckets)
{
    for (;;) {
        PAS_ASSERT(!buckets->size);
    
        hashtable* table = ensure_hashtable();

        PAS_ASSERT(table);

        unsigned index;
        for (index = table->size; index--;) {
            bucket** bucket_ptr = table->data + index;

            for (;;) {
                bucket* my_bucket = *bucket_ptr;

                if (!my_bucket) {
                    my_bucket = bucket_create();
                    if (!pas_compare_and_swap_ptr_weak(bucket_ptr, NULL, my_bucket)) {
                        bucket_destroy(my_bucket);
                        continue;
                    }
                }

                ptr_array_add(buckets, my_bucket);
                break;
            }
        }

        PAS_ASSERT(buckets->size);

        qsort(buckets->array, buckets->size, sizeof(void*), compare_ptr);

        for (index = 0; index < buckets->size; ++index)
            pas_lock_lock(&((bucket*)buckets->array[index])->lock);

        if (the_table == table)
            return;

        for (index = buckets->size; index--;)
            pas_lock_unlock(&((bucket*)buckets->array[index])->lock);
        buckets->size = 0;
    }
}

static void unlock_hashtable(ptr_array* buckets)
{
    unsigned index;
    for (index = buckets->size; index--;)
        pas_lock_unlock(&((bucket*)buckets->array[index])->lock);
}

static void ensure_hashtable_size(unsigned num_threads)
{
    hashtable* old_table = the_table;
    if (old_table && (double)old_table->size / (double)num_threads >= max_load_factor) {
        if (verbose) {
            pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": no need to rehash because %u / %u >= %u\n",
                    pas_get_current_system_thread_id(), old_table->size, num_threads, max_load_factor);
        }
        return;
    }

    ptr_array buckets;
    ptr_array_construct(&buckets);
    lock_hashtable(&buckets);

    old_table = the_table;
    PAS_ASSERT(old_table);

    if ((double)old_table->size / (double)num_threads >= max_load_factor) {
        if (verbose) {
            pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": after locking, no need to rehash "
                    "because %u / %u >= %u\n",
                    pas_get_current_system_thread_id(), old_table->size, num_threads, max_load_factor);
        }
        unlock_hashtable(&buckets);
        ptr_array_destruct(&buckets);
        return;
    }

    if (verbose) {
        pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": after locking, rehashing because %u / %u < %u\n",
                pas_get_current_system_thread_id(), old_table->size, num_threads, max_load_factor);
    }

    ptr_array thread_datas;
    ptr_array_construct(&thread_datas);
    unsigned index;
    for (index = 0; index < buckets.size; ++index) {
        bucket* my_bucket = (bucket*)buckets.array[index];
        thread_data* data;
        while ((data = bucket_dequeue(my_bucket)))
            ptr_array_add(&thread_datas, data);
    }

    unsigned new_size;
    PAS_ASSERT(!pas_mul_uint32_overflow(num_threads, growth_factor, &new_size));
    PAS_ASSERT(!pas_mul_uint32_overflow(new_size, max_load_factor, &new_size));
    if (verbose) {
        pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": creating table with size %u\n",
                pas_get_current_system_thread_id(), new_size);
    }

    hashtable* new_table = hashtable_create(new_size);
    unsigned bucket_reuse_index = buckets.size;
    for (index = thread_datas.size; index--;) {
        thread_data* data = (thread_data*)thread_datas.array[index];
        unsigned hash = pas_hash_ptr(data->address);
        unsigned index = hash % new_table->size;
        bucket* my_bucket = new_table->data[index];
        if (!my_bucket) {
            if (!bucket_reuse_index)
                my_bucket = bucket_create();
            else
                my_bucket = buckets.array[--bucket_reuse_index];
            new_table->data[index] = my_bucket;
        }
        bucket_enqueue(my_bucket, data);
    }

    if (verbose) {
        pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": bucket_reuse_index = %u\n",
                pas_get_current_system_thread_id(), bucket_reuse_index);
    }

    for (index = 0; index < new_table->size && bucket_reuse_index; ++index) {
        bucket** bucket_ptr = new_table->data + index;
        if (*bucket_ptr)
            continue;
        *bucket_ptr = buckets.array[--bucket_reuse_index];
    }

    PAS_ASSERT(!bucket_reuse_index);

    PAS_ASSERT(pas_compare_and_swap_ptr_strong(&the_table, old_table, new_table) == old_table);

    unlock_hashtable(&buckets);
    ptr_array_destruct(&buckets);
    ptr_array_destruct(&thread_datas);
}

static thread_data* thread_data_create(void)
{
    thread_data* result = (thread_data*)filc_allocate_utility(sizeof(thread_data));
    result->ref_count = 1;
    pas_system_mutex_construct(&result->lock);
    pas_system_condition_construct(&result->condition);
    result->address = NULL;
    result->next_in_queue = NULL;
    
    unsigned current_num_threads;
    for (;;) {
        unsigned old_num_threads = num_threads;
        PAS_ASSERT(!pas_add_uint32_overflow(old_num_threads, 1, &current_num_threads));
        if (pas_compare_and_swap_uint32_weak(&num_threads, old_num_threads, current_num_threads))
            break;
    }

    ensure_hashtable_size(current_num_threads);

    return result;
}

static void thread_data_destroy(thread_data* data)
{
    PAS_ASSERT(!data->ref_count);
    pas_system_mutex_destruct(&data->lock);
    pas_system_condition_destruct(&data->condition);
    filc_deallocate_yolo(data);

    for (;;) {
        unsigned old_num_threads = num_threads;
        PAS_ASSERT(old_num_threads);
        if (pas_compare_and_swap_uint32_weak(&num_threads, old_num_threads, old_num_threads - 1))
            break;
    }
}

static void thread_data_ref(thread_data* data)
{
    for (;;) {
        unsigned old_ref_count = data->ref_count;
        unsigned new_ref_count;
        PAS_ASSERT(!pas_add_uint32_overflow(old_ref_count, 1, &new_ref_count));
        if (pas_compare_and_swap_uint32_weak(&data->ref_count, old_ref_count, new_ref_count))
            return;
    }
}

static void thread_data_deref(thread_data* data)
{
    for (;;) {
        unsigned old_ref_count = data->ref_count;
        PAS_ASSERT(old_ref_count);
        unsigned new_ref_count = old_ref_count - 1;
        if (pas_compare_and_swap_uint32_weak(&data->ref_count, old_ref_count, new_ref_count)) {
            if (!new_ref_count)
                thread_data_destroy(data);
            return;
        }
    }
}

static pthread_key_t thread_data_key;
static pas_system_once thread_data_key_once;

static void thread_data_key_destructor(void* arg)
{
    thread_data* data;

    data = (thread_data*)arg;

    thread_data_deref(data);
}

static void initialize_thread_data_key(void)
{
    PAS_ASSERT(!pthread_key_create(&thread_data_key, thread_data_key_destructor));
}

static thread_data* my_thread_data(void)
{
    pas_system_once_run(&thread_data_key_once, initialize_thread_data_key);

    thread_data* data = (thread_data*)pthread_getspecific(thread_data_key);
    if (!data) {
        data = thread_data_create();
        pthread_setspecific(thread_data_key, data);
    }
    
    return data;
}

static bool enqueue(const void* address, thread_data* (*callback)(void* arg), void* arg)
{
    unsigned hash = pas_hash_ptr(address);

    for (;;) {
        hashtable* table = ensure_hashtable();
        unsigned index = hash % table->size;
        bucket** bucket_ptr = table->data + index;
        bucket* my_bucket;
        for (;;) {
            my_bucket = *bucket_ptr;
            if (!my_bucket) {
                my_bucket = bucket_create();
                if (!pas_compare_and_swap_ptr_weak(bucket_ptr, NULL, my_bucket)) {
                    bucket_destroy(my_bucket);
                    continue;
                }
            }
            break;
        }
        if (verbose) {
            pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": enqueueing onto bucket %p with index %u for address %p "
                    "with hash %u\n",
                    pas_get_current_system_thread_id(), my_bucket, index, address, hash);
        }
        PAS_ASSERT(my_bucket);
        pas_lock_lock(&my_bucket->lock);
        if (the_table != table) {
            pas_lock_unlock(&my_bucket->lock);
            continue;
        }

        thread_data* data = callback(arg);
        bool result;
        if (data) {
            if (verbose) {
                pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": proceeding to enqueue %p\n",
                        pas_get_current_system_thread_id(), data);
            }
            bucket_enqueue(my_bucket, data);
            result = true;
        } else
            result = false;
        pas_lock_unlock(&my_bucket->lock);
        return result;
    }
}

enum bucket_mode {
    bucket_ensure_non_empty,
    bucket_ignore_empty
};

typedef enum bucket_mode bucket_mode;

static bool dequeue(const void* address, bucket_mode mode,
                    dequeue_result (*callback)(thread_data* data, void* arg),
                    void (*finish)(bool result, void* arg),
                    void* arg)
{
    unsigned hash = pas_hash_ptr(address);

    for (;;) {
        hashtable* table = ensure_hashtable();
        unsigned index = hash % table->size;
        bucket** bucket_ptr = table->data + index;
        bucket* my_bucket = *bucket_ptr;
        if (!my_bucket) {
            if (mode == bucket_ignore_empty)
                return false;

            for (;;) {
                my_bucket = *bucket_ptr;
                if (!my_bucket) {
                    my_bucket = bucket_create();
                    if (!pas_compare_and_swap_ptr_weak(bucket_ptr, NULL, my_bucket)) {
                        bucket_destroy(my_bucket);
                        continue;
                    }
                }
                break;
            }
        }
        PAS_ASSERT(my_bucket);
        pas_lock_lock(&my_bucket->lock);
        if (the_table != table) {
            pas_lock_unlock(&my_bucket->lock);
            continue;
        }

        bucket_generic_dequeue(my_bucket, callback, arg);
        bool result = !!my_bucket->queue_head;
        finish(result, arg);
        pas_lock_unlock(&my_bucket->lock);
        return result;
    }
}

typedef struct {
    thread_data* me;
    const void* address;
    bool (*validate)(void* arg);
    void* arg;
    bool did_dequeue;
} park_data;

static thread_data* park_enqueue_callback(void* arg)
{
    park_data* data = (park_data*)arg;

    if (!data->validate(data->arg))
        return NULL;

    data->me->address = data->address;
    return data->me;
}

static dequeue_result park_dequeue_callback(thread_data* element, void* arg)
{
    park_data* data = (park_data*)arg;
    if (element == data->me) {
        data->did_dequeue = true;
        return dequeue_remove_and_stop;
    }
    return dequeue_ignore;
}

static void empty_finish_callback(bool result, void* arg)
{
    PAS_UNUSED_PARAM(result);
    PAS_UNUSED_PARAM(arg);
}

bool filc_park_conditionally(
    const void* address,
    bool (*validate)(void* arg),
    void (*before_sleep)(void* arg),
    void* arg,
    double absolute_timeout_milliseconds)
{
    if (verbose) {
        pas_log(PAS_SYSTEM_THREAD_ID_FORMAT ": parking.\n",
                pas_get_current_system_thread_id());
    }

    park_data data;
    data.address = address;
    data.validate = validate;
    data.arg = arg;
        
    data.me = my_thread_data();
    /* Guard against someone calling park_conditionally recursively from before_sleep. */
    PAS_ASSERT(!data.me->address);

    bool enqueue_result = enqueue(address, park_enqueue_callback, &data);

    if (!enqueue_result)
        return false;

    before_sleep(arg);

    bool did_get_dequeued;
    filc_exit();
    pas_system_mutex_lock(&data.me->lock);
    while (data.me->address
           && pas_get_time_in_milliseconds_for_system_condition() < absolute_timeout_milliseconds) {
        pas_system_condition_timed_wait(&data.me->condition, &data.me->lock, absolute_timeout_milliseconds);

        /* It's possible for the OS to decide not to wait. If it does that then it will also
           decide not to release the lock. If there's a bug in the time math, then this could
           result in a deadlock. Flashing the lock means that at worst it's just a CPU-eating
           spin. */
        pas_system_mutex_unlock(&data.me->lock);
        pas_system_mutex_lock(&data.me->lock);
    }
    PAS_ASSERT(!data.me->address || data.me->address == address);
    did_get_dequeued = !data.me->address;
    pas_system_mutex_unlock(&data.me->lock);
    filc_enter();

    if (did_get_dequeued) {
        /* This is the normal case - we got dequeued by someone before the timer expired. */
        return true;
    }

    data.did_dequeue = false;
    dequeue(address, bucket_ignore_empty, park_dequeue_callback, empty_finish_callback, &data);

    PAS_ASSERT(!data.me->next_in_queue);

    filc_exit();
    pas_system_mutex_lock(&data.me->lock);
    if (!data.did_dequeue) {
        while (data.me->address)
            pas_system_condition_wait(&data.me->condition, &data.me->lock);
    }
    data.me->address = NULL;
    pas_system_mutex_unlock(&data.me->lock);
    filc_enter();

    return !data.did_dequeue;
}

typedef struct {
    const void* address;
    thread_data* target;
    void (*callback)(filc_unpark_result result, void* arg);
    void* arg;
    bool did_call_finish;
} unpark_one_data;

static dequeue_result unpark_one_dequeue_callback(thread_data* element, void* arg)
{
    unpark_one_data* data = (unpark_one_data*)arg;
    if (element->address != data->address)
        return dequeue_ignore;
    thread_data_ref(element);
    data->target = element;
    return dequeue_remove_and_stop;
}

static void unpark_one_finish_callback(bool may_have_more_threads, void* arg)
{
    unpark_one_data* data = (unpark_one_data*)arg;
    filc_unpark_result result;
    result.did_unpark_thread = !!data->target;
    result.may_have_more_threads = result.did_unpark_thread && may_have_more_threads;
    data->callback(result, data->arg);
    data->did_call_finish = true;
}

void filc_unpark_one(
    const void* address,
    void (*callback)(filc_unpark_result result, void* arg),
    void* arg)
{
    unpark_one_data data;
    data.address = address;
    data.target = NULL;
    data.callback = callback;
    data.arg = arg;
    data.did_call_finish = false;

    dequeue(address, bucket_ensure_non_empty, unpark_one_dequeue_callback, unpark_one_finish_callback,
            &data);

    if (!data.target)
        return;

    PAS_ASSERT(data.target->address == address);

    filc_exit();
    pas_system_mutex_lock(&data.target->lock);
    data.target->address = NULL;
    pas_system_mutex_unlock(&data.target->lock);
    pas_system_condition_broadcast(&data.target->condition);
    filc_enter();

    /* Note that we could have just signaled a dead thread, if there was a timeout. That's fine
       since we're using refcounting. */
    thread_data_deref(data.target);
}

typedef struct {
    const void* address;
    ptr_array array;
    unsigned count;
} unpark_data;

static dequeue_result unpark_dequeue_callback(thread_data* element, void* arg)
{
    unpark_data* data = (unpark_data*)arg;
    if (element->address != data->address)
        return dequeue_ignore;
    thread_data_ref(element);
    ptr_array_add(&data->array, element);
    PAS_ASSERT(data->array.size <= data->count);
    if (data->array.size == data->count)
        return dequeue_remove_and_stop;
    return dequeue_remove_and_continue;
}

unsigned filc_unpark(const void* address, unsigned count)
{
    unpark_data data;
    data.address = address;
    ptr_array_construct(&data.array);
    data.count = count;
    dequeue(address, bucket_ignore_empty, unpark_dequeue_callback, empty_finish_callback, &data);

    PAS_ASSERT(data.array.size <= count);

    unsigned index;
    for (index = data.array.size; index--;) {
        thread_data* target = (thread_data*)data.array.array[index];
        PAS_ASSERT(target->address == address);
        filc_exit();
        pas_system_mutex_lock(&target->lock);
        target->address = NULL;
        pas_system_mutex_unlock(&target->lock);
        pas_system_condition_broadcast(&target->condition);
        filc_enter();
        thread_data_deref(target);
    }

    unsigned result = data.array.size;
    ptr_array_destruct(&data.array);
    return result;
}

#endif /* LIBPAS_ENABLED */

