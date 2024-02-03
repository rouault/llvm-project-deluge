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

#ifndef DELUGE_PARKING_LOT_H
#define DELUGE_PARKING_LOT_H

#include "pas_utils.h"

PAS_BEGIN_EXTERN_C;

struct deluge_unpark_result;
typedef struct deluge_unpark_result deluge_unpark_result;

struct deluge_unpark_result {
    /* True if some thread was unparked. */
    bool did_unpark_thread;

    /* True if there may be more threads on this address. This may be conservatively true. */
    bool may_have_more_threads;
};

/* Parks the thread in a queue associated with the given address, which cannot be null. The
   parking only succeeds if the validate function returns true while the queue lock is held.
  
   If validate returns false, it will unlock the internal parking queue and then it will
   return false.
  
   If validate returns true, it will enqueue the thread, unlock the parking queue lock, call
   the before_sleep function, and then it will sleep so long as the thread continues to be on the
   queue and the timeout hasn't fired. Finally, this returns true if we actually got unparked or
   false if the timeout was hit.
  
   Note that before_sleep is called with no locks held, so it's OK to do pretty much anything so
   long as you don't recursively call park_conditionally(). You can call unpark_one()/unpark_all()
   though. It's useful to use before_sleep() for implementing condition variables. */
PAS_API bool deluge_park_conditionally(
    const void* address,
    bool (*validate)(void* arg),
    void (*before_sleep)(void* arg),
    void* arg,
    double absolute_timeout_milliseconds);

/* Unparks one thread from the queue associated with the given address, and calls the given
   callback while the address is locked. Reports to the callback whether any thread got
   unparked and whether there may be any other threads still on the queue. */
PAS_API void deluge_unpark_one(
    const void* address,
    void (*callback)(deluge_unpark_result result, void* arg),
    void* arg);

/* Unparks every thread from the queue associated with the given address, which cannot be null.
   Returns the number of threads unparked. */
PAS_API unsigned deluge_unpark_all(const void* address);

PAS_END_EXTERN_C;

#endif /* PAS_PARKING_LOT_H */

