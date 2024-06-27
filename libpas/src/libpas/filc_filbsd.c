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

#include "pas_config.h"

#if LIBPAS_ENABLED

#include "filc_runtime.h"

#if PAS_ENABLE_FILC && FILC_FILBSD

#include "bmalloc_heap.h"
#include "filc_native.h"
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <netdb.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <dirent.h>
#include <syslog.h>
#include <sys/wait.h>
#include <grp.h>
#include <sys/sysctl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <poll.h>
#include <spawn.h>

int filc_to_user_errno(int errno_value)
{
    return errno_value;
}

typedef struct {
    int result;
    int my_errno;
} ioctl_result;

static ioctl_result do_ioctl(filc_thread* my_thread, int fd, unsigned long request, void* ptr)
{
    ioctl_result result;
    filc_exit(my_thread);
    result.result = ioctl(fd, request, ptr);
    result.my_errno = errno;
    filc_enter(my_thread);
    return result;
}

static int handle_ioctl_result(ioctl_result result)
{
    if (result.result < 0) {
        PAS_ASSERT(result.result == -1);
        filc_set_errno(result.my_errno);
    }
    return result.result;
}

int filc_native_zsys_ioctl(filc_thread* my_thread, int fd, unsigned long request, filc_ptr args)
{
    if (filc_ptr_available(args) < FILC_WORD_SIZE)
        return handle_ioctl_result(do_ioctl(my_thread, fd, request, NULL));

    filc_ptr arg_ptr = filc_ptr_get_next_ptr(my_thread, &args);
    if (!filc_ptr_ptr(arg_ptr))
        return handle_ioctl_result(do_ioctl(my_thread, fd, request, NULL));

    size_t extent_limit = 128;
    char* base = (char*)filc_ptr_ptr(arg_ptr);
    filc_object* object = filc_ptr_object(arg_ptr);
    size_t maximum_extent = filc_ptr_available(arg_ptr);
    for(;;) {
        size_t limited_extent = pas_min_uintptr(maximum_extent, extent_limit);
        size_t index;
        for (index = 0; index < limited_extent; ++index) {
            filc_word_type word_type = filc_object_get_word_type(
                object, filc_object_word_type_index_for_ptr(object, base + index));
            if (word_type != FILC_WORD_TYPE_UNSET && word_type != FILC_WORD_TYPE_INT) {
                limited_extent = index;
                break;
            }
        }

        char* input_copy = bmalloc_allocate(limited_extent);
        for (index = 0; index < limited_extent; ++index) {
            char value = base[index];
            if (value)
                filc_check_read_int(filc_ptr_with_ptr(arg_ptr, base + index), 1, NULL);
            input_copy[index] = value;
        }

        char* start_of_space =
            filc_thread_get_end_of_space_with_guard_page_with_size(my_thread, limited_extent)
            - limited_extent;
        memcpy(start_of_space, input_copy, limited_extent);

        ioctl_result result = do_ioctl(my_thread, fd, request, start_of_space);
        if (result.result >= 0 || result.my_errno != EFAULT) {
            if (result.result >= 0) {
                for (index = 0; index < limited_extent; ++index) {
                    if (start_of_space[index] == input_copy[index])
                        continue;
                    filc_check_write_int(filc_ptr_with_ptr(arg_ptr, base + index), 1, NULL);
                    base[index] = start_of_space[index];
                }
            }
            bmalloc_deallocate(input_copy);
            return handle_ioctl_result(result);
        }

        PAS_ASSERT(result.result == -1 && result.my_errno == EFAULT);
        bmalloc_deallocate(input_copy);
        if (extent_limit > limited_extent) {
            filc_set_errno(EFAULT);
            return -1;
        }
        PAS_ASSERT(!pas_mul_uintptr_overflow(extent_limit, 2, &extent_limit));
    }
}

int filc_native_zsys_socket(filc_thread* my_thread, int domain, int type, int protocol)
{
    filc_exit(my_thread);
    int result = socket(domain, type, protocol);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_setsockopt(filc_thread* my_thread, int sockfd, int level, int optname,
                                filc_ptr optval_ptr, unsigned optlen)
{
    filc_check_read_int(optval_ptr, optlen, NULL);
    filc_pin(filc_ptr_object(optval_ptr));
    filc_exit(my_thread);
    int result = setsockopt(sockfd, level, optname, filc_ptr_ptr(optval_ptr), optlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(optval_ptr));
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_bind(filc_thread* my_thread, int sockfd, filc_ptr addr_ptr, unsigned addrlen)
{
    filc_check_read_int(addr_ptr, addrlen, NULL);
    filc_pin(filc_ptr_object(addr_ptr));
    filc_exit(my_thread);
    int result = bind(sockfd, (const struct sockaddr*)filc_ptr_ptr(addr_ptr), addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(addr_ptr));
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_connect(filc_thread* my_thread, int sockfd, filc_ptr addr_ptr, unsigned addrlen)
{
    filc_check_read_int(addr_ptr, addrlen, NULL);
    filc_pin(filc_ptr_object(addr_ptr));
    filc_exit(my_thread);
    int result = connst(sockfd, (const struct sockaddr*)filc_ptr_ptr(addr_ptr), addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(addr_ptr));
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

#endif /* PAS_ENABLE_FILC && FILC_FILBSD */

#endif /* LIBPAS_ENABLED */
