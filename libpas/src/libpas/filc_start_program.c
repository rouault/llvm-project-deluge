/*
 * Copyright (c) 2023-2024 Epic Games, Inc. All Rights Reserved.
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

#include "filc_native.h"
#include "filc_runtime.h"
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>

extern char** environ;

void filc_start_program(int argc, char** argv,
                        filc_ptr pizlonated___init_libc(filc_global_initialization_context*),
                        filc_ptr pizlonated_exit(filc_global_initialization_context*),
                        filc_ptr pizlonated_main(filc_global_initialization_context*))
{
    static const bool verbose = false;
    
    filc_ptr pizlonated_argv;
    int index;
    filc_ptr main_ptr;
    int environ_size;
    filc_ptr environ_ptr;
    filc_ptr __init_libc_ptr;
    filc_ptr exit_ptr;

    PAS_ASSERT(argc >= 1);

    filc_initialize();
    filc_thread* my_thread = filc_get_my_thread();
    filc_enter(my_thread);

    FILC_DEFINE_RUNTIME_ORIGIN(origin, "start_program", 0);

    struct {
        FILC_FRAME_BODY;
    } actual_frame;
    pas_zero_memory(&actual_frame, sizeof(actual_frame));
    filc_frame* frame = (filc_frame*)&actual_frame;
    frame->origin = &origin;
    filc_push_frame(my_thread, frame);

    filc_native_frame native_frame;
    filc_push_native_frame(my_thread, &native_frame);

    pizlonated_argv = filc_ptr_create(my_thread, filc_allocate(my_thread, sizeof(filc_ptr) * (argc + 1)));

    for (index = 0; index < argc; ++index) {
        filc_ptr arg;
        filc_ptr arg_ptr;
        size_t size;
        arg_ptr = filc_ptr_with_offset(pizlonated_argv, index * sizeof(filc_ptr));
        size = strlen(argv[index]) + 1;
        arg = filc_ptr_create(my_thread, filc_allocate_int(my_thread, size));
        memcpy(filc_ptr_ptr(arg), argv[index], size);
        filc_check_write_ptr(arg_ptr, NULL);
        filc_ptr_store(my_thread, (filc_ptr*)filc_ptr_ptr(arg_ptr), arg);
    }

    filc_ptr arg_ptr = filc_ptr_with_offset(pizlonated_argv, argc * sizeof(filc_ptr));
    filc_check_write_ptr(arg_ptr, NULL);
    filc_ptr_store(my_thread, (filc_ptr*)filc_ptr_ptr(arg_ptr), filc_ptr_forge_null());

    if (pizlonated___init_libc) {
        for (environ_size = 0; environ[environ_size]; ++environ_size);
        environ_size++;

        environ_ptr = filc_ptr_create(
            my_thread, filc_allocate(my_thread, sizeof(filc_ptr) * environ_size));

        for (index = 0; index < environ_size; ++index) {
            filc_ptr env_ptr = filc_ptr_with_offset(environ_ptr, index * sizeof(filc_ptr));
            filc_ptr env_value;
            if (index == environ_size - 1) {
                PAS_ASSERT(!environ[index]);
                env_value = filc_ptr_forge_null();
            } else {
                size_t size;
                filc_ptr env_copy;
                PAS_ASSERT(environ[index]);
                size = strlen(environ[index]) + 1;
                env_copy = filc_ptr_create(my_thread, filc_allocate_int(my_thread, size));
                filc_ptr_store(my_thread, (filc_ptr*)filc_ptr_ptr(env_ptr), env_copy);
                memcpy(filc_ptr_ptr(env_copy), environ[index], size);
            }
        }

        __init_libc_ptr = pizlonated___init_libc(NULL);
        if (verbose) {
            pas_log("__init_libc object = %p\n", filc_ptr_object(__init_libc_ptr));
            pas_log("__init_libc object->lower = %p\n", filc_ptr_object(__init_libc_ptr)->lower);
            pas_log("__init_libc object->upper = %p\n", filc_ptr_object(__init_libc_ptr)->upper);
            pas_log("__init_libc ptr = %p\n", filc_ptr_ptr(__init_libc_ptr));
        }
        filc_thread_track_object(my_thread, filc_ptr_object(__init_libc_ptr));
        filc_check_function_call(__init_libc_ptr);
        filc_call_user_void_ptr_ptr(
            my_thread, (bool (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(__init_libc_ptr),
            environ_ptr, filc_ptr_load(my_thread, (filc_ptr*)filc_ptr_ptr(pizlonated_argv)));
    }

    filc_run_deferred_global_ctors(my_thread);

    main_ptr = pizlonated_main(NULL);
    filc_thread_track_object(my_thread, filc_ptr_object(main_ptr));
    filc_check_function_call(main_ptr);
    int exit_status = filc_call_user_int_int_ptr(
        my_thread, (bool (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(main_ptr), argc, pizlonated_argv);

    if (verbose)
        pas_log("Exiting!\n");

    if (pizlonated_exit) {
        exit_ptr = pizlonated_exit(NULL);
        filc_thread_track_object(my_thread, filc_ptr_object(exit_ptr));
        filc_call_user_void_int(
            my_thread, (bool (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(exit_ptr), exit_status);
    } else
        exit(exit_status);

    PAS_ASSERT(!"Should not get here");
}

