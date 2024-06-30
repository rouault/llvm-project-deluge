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

/* FIXME: This is really a great implementation of the "not-musl" syscalls that make sense for all
   platforms.

   But we don't have to worry about that just yet. */
#if PAS_ENABLE_FILC && FILC_FILBSD

#include "bmalloc_heap.h"
#include "filc_native.h"
#include "fugc.h"
#include "pas_page_malloc.h"
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
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/ktrace.h>

static pas_lock roots_lock = PAS_LOCK_INITIALIZER;
static filc_object* profil_samples_root = NULL;

void filc_mark_user_global_roots(filc_object_array* mark_stack)
{
    pas_lock_lock(&roots_lock);
    fugc_mark(mark_stack, profil_samples_root);
    pas_lock_unlock(&roots_lock);
}

int filc_to_user_errno(int errno_value)
{
    return errno_value;
}

void filc_from_user_sigset(filc_user_sigset* user_sigset,
                           sigset_t* sigset)
{
    PAS_ASSERT(sizeof(filc_user_sigset) == sizeof(sigset_t));
    memcpy(sigset, user_sigset, sizeof(sigset_t));
}

void filc_to_user_sigset(sigset_t* sigset, filc_user_sigset* user_sigset)
{
    PAS_ASSERT(sizeof(sigset_t) == sizeof(filc_user_sigset));
    memcpy(user_sigset, sigset, sizeof(sigset_t));
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

static int handle_ioctl_result_excluding_fault(ioctl_result result)
{
    if (result.result < 0) {
        PAS_ASSERT(result.result == -1);
        PAS_ASSERT(result.my_errno != EFAULT);
        filc_set_errno(result.my_errno);
    }
    return result.result;
}

static int handle_ioctl_result(ioctl_result result, unsigned long request)
{
    if (result.result < 0 && result.my_errno == EFAULT) {
        filc_safety_panic(
            NULL,
            "passed NULL as the argument to ioctl request %lu but the kernel wanted a real argument.",
            request);
    }
    return handle_ioctl_result_excluding_fault(result);
}

int filc_native_zsys_ioctl(filc_thread* my_thread, int fd, unsigned long request, filc_ptr args)
{
    if (filc_ptr_available(args) < FILC_WORD_SIZE)
        return handle_ioctl_result(do_ioctl(my_thread, fd, request, NULL), request);

    filc_ptr arg_ptr = filc_ptr_get_next_ptr(my_thread, &args);
    if (!filc_ptr_ptr(arg_ptr))
        return handle_ioctl_result(do_ioctl(my_thread, fd, request, NULL), request);

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
            return handle_ioctl_result_excluding_fault(result);
        }

        PAS_ASSERT(result.result == -1 && result.my_errno == EFAULT);
        bmalloc_deallocate(input_copy);
        if (extent_limit > limited_extent) {
            filc_safety_panic(
                NULL,
                "was only able to pass %zu int bytes to ioctl request %lu but the kernel wanted more "
                "(arg ptr = %s).",
                extent_limit, request, filc_ptr_to_new_string(arg_ptr));
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
    int result = connect(sockfd, (const struct sockaddr*)filc_ptr_ptr(addr_ptr), addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(addr_ptr));
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_getsockname(filc_thread* my_thread, int sockfd, filc_ptr addr_ptr,
                                 filc_ptr addrlen_ptr)
{
    filc_check_read_int(addrlen_ptr, sizeof(unsigned), NULL);
    unsigned addrlen = *(unsigned*)filc_ptr_ptr(addrlen_ptr);

    filc_check_write_int(addr_ptr, addrlen, NULL);

    filc_pin(filc_ptr_object(addr_ptr));
    filc_exit(my_thread);
    int result = getsockname(sockfd, (struct sockaddr*)filc_ptr_ptr(addr_ptr), &addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(addr_ptr));

    if (result < 0)
        filc_set_errno(my_errno);
    else {
        filc_check_write_int(addrlen_ptr, sizeof(unsigned), NULL);
        *(unsigned*)filc_ptr_ptr(addrlen_ptr) = addrlen;
    }
    return result;
}

int filc_native_zsys_getsockopt(filc_thread* my_thread, int sockfd, int level, int optname,
                                filc_ptr optval_ptr, filc_ptr optlen_ptr)
{
    filc_check_read_int(optlen_ptr, sizeof(unsigned), NULL);
    unsigned optlen = *(unsigned*)filc_ptr_ptr(optlen_ptr);

    filc_check_write_int(optval_ptr, optlen, NULL);

    filc_pin(filc_ptr_object(optval_ptr));
    filc_exit(my_thread);
    int result = getsockopt(sockfd, level, optname, filc_ptr_ptr(optval_ptr), &optlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(optval_ptr));

    if (result < 0)
        filc_set_errno(my_errno);
    else {
        filc_check_write_int(optlen_ptr, sizeof(unsigned), NULL);
        *(unsigned*)filc_ptr_ptr(optlen_ptr) = optlen;
    }
    return result;
}

int filc_native_zsys_getpeername(filc_thread* my_thread, int sockfd, filc_ptr addr_ptr,
                                 filc_ptr addrlen_ptr)
{
    filc_check_read_int(addrlen_ptr, sizeof(unsigned), NULL);
    unsigned addrlen = *(unsigned*)filc_ptr_ptr(addrlen_ptr);

    filc_check_write_int(addr_ptr, addrlen, NULL);

    filc_pin(filc_ptr_object(addr_ptr));
    filc_exit(my_thread);
    int result = getpeername(sockfd, (struct sockaddr*)filc_ptr_ptr(addr_ptr), &addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(addr_ptr));

    if (result < 0)
        filc_set_errno(my_errno);
    else {
        filc_check_write_int(addrlen_ptr, sizeof(unsigned), NULL);
        *(unsigned*)filc_ptr_ptr(addrlen_ptr) = addrlen;
    }
    return result;
}

ssize_t filc_native_zsys_sendto(filc_thread* my_thread, int sockfd, filc_ptr buf_ptr, size_t len,
                                int flags, filc_ptr addr_ptr, unsigned addrlen)
{
    filc_check_read_int(buf_ptr, len, NULL);
    filc_check_read_int(addr_ptr, addrlen, NULL);
    filc_pin(filc_ptr_object(buf_ptr));
    filc_pin(filc_ptr_object(addr_ptr));
    filc_exit(my_thread);
    int result = sendto(sockfd, filc_ptr_ptr(buf_ptr), len, flags,
                        (const struct sockaddr*)filc_ptr_ptr(addr_ptr), addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf_ptr));
    filc_unpin(filc_ptr_object(addr_ptr));
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

ssize_t filc_native_zsys_recvfrom(filc_thread* my_thread, int sockfd, filc_ptr buf_ptr, size_t len,
                                  int flags, filc_ptr addr_ptr, filc_ptr addrlen_ptr)
{
    filc_check_write_int(buf_ptr, len, NULL);
    filc_check_read_int(addrlen_ptr, sizeof(unsigned), NULL);
    unsigned addrlen = *(unsigned*)filc_ptr_ptr(addrlen_ptr);
    filc_check_write_int(addr_ptr, addrlen, NULL);
    filc_pin(filc_ptr_object(buf_ptr));
    filc_pin(filc_ptr_object(addr_ptr));
    filc_exit(my_thread);
    int result = recvfrom(sockfd, filc_ptr_ptr(buf_ptr), len, flags,
                          (struct sockaddr*)filc_ptr_ptr(addr_ptr), &addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf_ptr));
    filc_unpin(filc_ptr_object(addr_ptr));
    if (result < 0)
        filc_set_errno(my_errno);
    else {
        filc_check_write_int(addrlen_ptr, sizeof(unsigned), NULL);
        *(unsigned*)filc_ptr_ptr(addrlen_ptr) = addrlen;
    }
    return result;
}

int filc_native_zsys_accept(filc_thread* my_thread, int sockfd, filc_ptr addr_ptr, filc_ptr addrlen_ptr)
{
    filc_check_read_int(addrlen_ptr, sizeof(unsigned), NULL);
    unsigned addrlen = *(unsigned*)filc_ptr_ptr(addrlen_ptr);
    filc_check_write_int(addr_ptr, addrlen, NULL);
    filc_pin(filc_ptr_object(addr_ptr));
    filc_exit(my_thread);
    int result = accept(sockfd, (struct sockaddr*)filc_ptr_ptr(addr_ptr), &addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(addr_ptr));
    if (result < 0)
        filc_set_errno(my_errno);
    else {
        filc_check_write_int(addrlen_ptr, sizeof(unsigned), NULL);
        *(unsigned*)filc_ptr_ptr(addrlen_ptr) = addrlen;
    }
    return result;
}

int filc_native_zsys_socketpair(filc_thread* my_thread, int domain, int type, int protocol,
                                filc_ptr sv_ptr)
{
    filc_check_write_int(sv_ptr, sizeof(int) * 2, NULL);
    filc_pin(filc_ptr_object(sv_ptr));
    filc_exit(my_thread);
    int result = socketpair(domain, type, protocol, (int*)filc_ptr_ptr(sv_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(sv_ptr));
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

struct user_msghdr {
    filc_ptr msg_name;
    unsigned msg_namelen;
    filc_ptr msg_iov;
    int msg_iovlen;
    filc_ptr msg_control;
    unsigned msg_controllen;
    int msg_flags;
};

static void check_user_msghdr(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_PTR_FIELD(ptr, struct user_msghdr, msg_name, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_msghdr, msg_namelen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_msghdr, msg_iov, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_msghdr, msg_iovlen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_msghdr, msg_control, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_msghdr, msg_controllen, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_msghdr, msg_flags, access_kind);
}

static void destroy_msghdr(struct msghdr* msghdr)
{
    filc_unprepare_iovec(msghdr->msg_iov);
}

static void from_user_msghdr_base(filc_thread* my_thread,
                                  struct user_msghdr* user_msghdr, struct msghdr* msghdr,
                                  filc_access_kind access_kind)
{
    pas_zero_memory(msghdr, sizeof(struct msghdr));

    int iovlen = user_msghdr->msg_iovlen;
    msghdr->msg_iov = filc_prepare_iovec(
        my_thread, filc_ptr_load(my_thread, &user_msghdr->msg_iov), iovlen, access_kind);
    msghdr->msg_iovlen = iovlen;

    msghdr->msg_flags = user_msghdr->msg_flags;
}

static void from_user_msghdr_impl(filc_thread* my_thread,
                                  struct user_msghdr* user_msghdr, struct msghdr* msghdr,
                                  filc_access_kind access_kind)
{
    from_user_msghdr_base(my_thread, user_msghdr, msghdr, access_kind);

    unsigned msg_namelen = user_msghdr->msg_namelen;
    if (msg_namelen) {
        filc_ptr msg_name = filc_ptr_load(my_thread, &user_msghdr->msg_name);
        filc_check_access_int(msg_name, msg_namelen, access_kind, NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(msg_name));
        msghdr->msg_name = filc_ptr_ptr(msg_name);
        msghdr->msg_namelen = msg_namelen;
    }

    unsigned user_controllen = user_msghdr->msg_controllen;
    if (user_controllen) {
        filc_ptr user_control = filc_ptr_load(my_thread, &user_msghdr->msg_control);
        filc_check_access_int(user_control, user_controllen, access_kind, NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(user_control));
        msghdr->msg_control = filc_ptr_ptr(user_control);
        msghdr->msg_controllen = user_controllen;
    }
}

static void from_user_msghdr_for_send(filc_thread* my_thread,
                                      struct user_msghdr* user_msghdr, struct msghdr* msghdr)
{
    from_user_msghdr_impl(my_thread, user_msghdr, msghdr, filc_read_access);
}

static void from_user_msghdr_for_recv(filc_thread* my_thread,
                                      struct user_msghdr* user_msghdr, struct msghdr* msghdr)
{
    from_user_msghdr_impl(my_thread, user_msghdr, msghdr, filc_write_access);
}

static void to_user_msghdr_for_recv(struct msghdr* msghdr, struct user_msghdr* user_msghdr)
{
    user_msghdr->msg_namelen = msghdr->msg_namelen;
    user_msghdr->msg_controllen = msghdr->msg_controllen;
    user_msghdr->msg_flags = msghdr->msg_flags;
}

ssize_t filc_native_zsys_sendmsg(filc_thread* my_thread, int sockfd, filc_ptr msg_ptr, int flags)
{
    static const bool verbose = false;
    
    if (verbose)
        pas_log("In sendmsg\n");

    check_user_msghdr(msg_ptr, filc_read_access);
    struct user_msghdr* user_msg = (struct user_msghdr*)filc_ptr_ptr(msg_ptr);
    struct msghdr msg;
    from_user_msghdr_for_send(my_thread, user_msg, &msg);
    if (verbose)
        pas_log("Got this far\n");
    filc_exit(my_thread);
    if (verbose)
        pas_log("Actually doing sendmsg\n");
    ssize_t result = sendmsg(sockfd, &msg, flags);
    int my_errno = errno;
    if (verbose)
        pas_log("sendmsg result = %ld\n", (long)result);
    filc_enter(my_thread);
    destroy_msghdr(&msg);
    if (result < 0)
        filc_set_errno(errno);
    return result;
}

ssize_t filc_native_zsys_recvmsg(filc_thread* my_thread, int sockfd, filc_ptr msg_ptr, int flags)
{
    static const bool verbose = false;

    check_user_msghdr(msg_ptr, filc_read_access);
    struct user_msghdr* user_msg = (struct user_msghdr*)filc_ptr_ptr(msg_ptr);
    struct msghdr msg;
    from_user_msghdr_for_recv(my_thread, user_msg, &msg);
    filc_exit(my_thread);
    if (verbose) {
        pas_log("Actually doing recvmsg\n");
        pas_log("msg.msg_iov = %p\n", msg.msg_iov);
        pas_log("msg.msg_iovlen = %d\n", msg.msg_iovlen);
        int index;
        for (index = 0; index < (int)msg.msg_iovlen; ++index)
            pas_log("msg.msg_iov[%d].iov_len = %zu\n", index, msg.msg_iov[index].iov_len);
    }
    ssize_t result = recvmsg(sockfd, &msg, flags);
    int my_errno = errno;
    if (verbose)
        pas_log("recvmsg result = %ld\n", (long)result);
    filc_enter(my_thread);
    if (result < 0) {
        if (verbose)
            pas_log("recvmsg failed: %s\n", strerror(errno));
        filc_set_errno(my_errno);
    } else {
        check_user_msghdr(msg_ptr, filc_write_access);
        to_user_msghdr_for_recv(&msg, user_msg);
    }
    destroy_msghdr(&msg);
    return result;

einval:
    filc_set_errno(EINVAL);
    return -1;
}

int filc_native_zsys_fcntl(filc_thread* my_thread, int fd, int cmd, filc_ptr args)
{
    static const bool verbose = false;
    
    bool have_arg_int = false;
    bool have_arg_flock = false;
    filc_access_kind arg_flock_access_kind = filc_read_access;
    bool have_arg_kinfo_file = false;
    switch (cmd) {
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_DUP2FD:
    case F_DUP2FD_CLOEXEC:
    case F_SETFD:
    case F_SETFL:
    case F_SETOWN:
    case F_READAHEAD:
    case F_RDAHEAD:
    case F_ADD_SEALS:
        have_arg_int = true;
        break;
    case F_GETLK:
        have_arg_flock = true;
        arg_flock_access_kind = filc_write_access;
        break;
    case F_SETLK:
    case F_SETLKW:
        have_arg_flock = true;
        break;
    case F_KINFO:
        have_arg_kinfo_file = true;
        break;
    case F_GETFD:
    case F_GETFL:
    case F_GETOWN:
    case F_GET_SEALS:
    case F_ISUNIONSTACK:
        break;
    default:
        if (verbose)
            pas_log("no cmd!\n");
        filc_set_errno(EINVAL);
        return -1;
    }
    int arg_int = 0;
    void* arg_ptr = NULL;
    if (have_arg_int)
        arg_int = filc_ptr_get_next_int(&args);
    else if (have_arg_flock) {
        filc_ptr flock_ptr = filc_ptr_get_next_ptr(my_thread, &args);
        filc_check_access_int(flock_ptr, sizeof(struct flock), arg_flock_access_kind, NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(flock_ptr));
        arg_ptr = filc_ptr_ptr(flock_ptr);
    } else if (have_arg_kinfo_file) {
        filc_ptr kinfo_file_ptr = filc_ptr_get_next_ptr(my_thread, &args);
        filc_check_write_int(kinfo_file_ptr, sizeof(struct kinfo_file), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(kinfo_file_ptr));
        arg_ptr = filc_ptr_ptr(kinfo_file_ptr);
    }
    if (verbose)
        pas_log("so far so good.\n");
    int result;
    filc_exit(my_thread);
    if (have_arg_int)
        result = fcntl(fd, cmd, arg_int);
    else if (arg_ptr)
        result = fcntl(fd, cmd, arg_ptr);
    else
        result = fcntl(fd, cmd);
    int my_errno = errno;
    filc_enter(my_thread);
    if (verbose)
        pas_log("result = %d\n", result);
    if (result == -1) {
        if (verbose)
            pas_log("error = %s\n", strerror(my_errno));
        filc_set_errno(my_errno);
    }
    return result;
}

int filc_native_zsys_poll(filc_thread* my_thread, filc_ptr pollfds_ptr, unsigned long nfds, int timeout)
{
    filc_check_write_int(pollfds_ptr, sizeof(struct pollfd) * nfds, NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(pollfds_ptr));
    struct pollfd* pollfds = (struct pollfd*)filc_ptr_ptr(pollfds_ptr);
    filc_exit(my_thread);
    int result = poll(pollfds, nfds, timeout);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_unmount(filc_thread* my_thread, filc_ptr dir_ptr, int flags)
{
    char* dir = filc_check_and_get_new_str(dir_ptr);
    filc_exit(my_thread);
    int result = unmount(dir, flags);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    bmalloc_deallocate(dir);
    return result;
}

int filc_native_zsys_nmount(filc_thread* my_thread, filc_ptr iov_ptr, unsigned niov, int flags)
{
    /* This syscall is the coolest thing ever. */
    struct iovec* iov = filc_prepare_iovec(my_thread, iov_ptr, niov, filc_read_access);
    filc_exit(my_thread);
    int result = nmount(iov, niov, flags);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    filc_unprepare_iovec(iov);
    return result;
}

int filc_native_zsys_chflags(filc_thread* my_thread, filc_ptr path_ptr, unsigned long flags)
{
    char* path = filc_check_and_get_new_str(path_ptr);
    filc_exit(my_thread);
    int result = chflags(path, flags);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    bmalloc_deallocate(path);
    return result;
}

int filc_native_zsys_fchflags(filc_thread* my_thread, int fd, unsigned long flags)
{
    filc_exit(my_thread);
    int result = fchflags(fd, flags);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_profil(filc_thread* my_thread, filc_ptr samples_ptr, size_t size,
                            unsigned long offset, int scale)
{
    PAS_UNUSED_PARAM(my_thread);
    if (filc_ptr_ptr(samples_ptr)) {
        filc_check_write_int(samples_ptr, size, NULL);
        filc_pin(filc_ptr_object(samples_ptr));
    }
    pas_lock_lock(&roots_lock);
    int result = profil((char*)filc_ptr_ptr(samples_ptr), size, offset, scale);
    PAS_ASSERT(!result || result == -1);
    int my_errno = errno;
    if (!result) {
        filc_unpin(profil_samples_root);
        profil_samples_root = filc_ptr_ptr(samples_ptr) ? filc_ptr_object(samples_ptr) : NULL;
    } else if (filc_ptr_ptr(samples_ptr))
        filc_unpin(filc_ptr_object(samples_ptr));
    pas_lock_unlock(&roots_lock);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_setlogin(filc_thread* my_thread, filc_ptr name_ptr)
{
    char* name = filc_check_and_get_new_str(name_ptr);
    filc_exit(my_thread);
    int result = setlogin(name);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    bmalloc_deallocate(name);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_acct(filc_thread* my_thread, filc_ptr file_ptr)
{
    char* file = filc_check_and_get_new_str(file_ptr);
    filc_exit(my_thread);
    int result = acct(file);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    bmalloc_deallocate(file);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_reboot(filc_thread* my_thread, int howto)
{
    filc_exit(my_thread);
    int result = reboot(howto);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(result == -1);
    filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_revoke(filc_thread* my_thread, filc_ptr path_ptr)
{
    char* path = filc_check_and_get_new_str(path_ptr);
    filc_exit(my_thread);
    int result = revoke(path);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    bmalloc_deallocate(path);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_ktrace(filc_thread* my_thread, filc_ptr tracefile_ptr, int ops, int trpoints,
                            int pid)
{
    char* tracefile = filc_check_and_get_new_str(tracefile_ptr);
    filc_exit(my_thread);
    int result = ktrace(tracefile, ops, trpoints, pid);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    bmalloc_deallocate(tracefile);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_setgroups(filc_thread* my_thread, int size, filc_ptr list_ptr)
{
    static const bool verbose = false;
    
    size_t total_size;
    FILC_CHECK(
        !pas_mul_uintptr_overflow(sizeof(unsigned), size, &total_size),
        NULL,
        "size argument too big, causes overflow; size = %d.",
        size);
    filc_check_read_int(list_ptr, total_size, NULL);
    filc_pin(filc_ptr_object(list_ptr));
    filc_exit(my_thread);
    PAS_ASSERT(sizeof(gid_t) == sizeof(unsigned));
    if (verbose)
        pas_log("size = %d\n", size);
    int result = setgroups(size, (gid_t*)filc_ptr_ptr(list_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(list_ptr));
    if (verbose)
        pas_log("result = %d, error = %s\n", result, strerror(my_errno));
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_madvise(filc_thread* my_thread, filc_ptr addr_ptr, size_t len, int behav)
{
    filc_check_access_common(addr_ptr, len, filc_write_access, NULL);
    filc_check_pin_and_track_mmap(my_thread, addr_ptr);
    filc_exit(my_thread);
    int result = madvise(filc_ptr_ptr(addr_ptr), len, behav);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_mincore(filc_thread* my_thread, filc_ptr addr, size_t len, filc_ptr vec_ptr)
{
    filc_check_write_int(
        vec_ptr,
        pas_round_up_to_power_of_2(
            len, pas_page_malloc_alignment()) >> pas_page_malloc_alignment_shift(),
        NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(vec_ptr));
    filc_exit(my_thread);
    int result = mincore(filc_ptr_ptr(addr), len, (char*)filc_ptr_ptr(vec_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_swapon(filc_thread* my_thread, filc_ptr special_ptr)
{
    char* special = filc_check_and_get_new_str(special_ptr);
    filc_exit(my_thread);
    int result = swapon(special);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(special);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_swapoff(filc_thread* my_thread, filc_ptr special_ptr, unsigned flags)
{
    char* special = filc_check_and_get_new_str(special_ptr);
    filc_exit(my_thread);
    int result = swapoff(special, flags);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(special);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_getdtablesize(filc_thread* my_thread)
{
    filc_exit(my_thread);
    int result = getdtablesize();
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_getpriority(filc_thread* my_thread, int which, int who)
{
    filc_exit(my_thread);
    errno = 0;
    int result = getpriority(which, who);
    int my_errno = errno;
    filc_enter(my_thread);
    if (my_errno)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_setpriority(filc_thread* my_thread, int which, int who, int prio)
{
    filc_exit(my_thread);
    int result = setpriority(which, who, prio);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_gettimeofday(filc_thread* my_thread, filc_ptr tp_ptr, filc_ptr tzp_ptr)
{
    filc_check_write_int(tp_ptr, sizeof(struct timeval), NULL);
    filc_check_write_int(tzp_ptr, sizeof(struct timezone), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(tp_ptr));
    filc_pin_tracked(my_thread, filc_ptr_object(tzp_ptr));
    filc_exit(my_thread);
    int result = gettimeofday((struct timeval*)filc_ptr_ptr(tp_ptr),
                              (struct timezone*)filc_ptr_ptr(tzp_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_settimeofday(filc_thread* my_thread, filc_ptr tp_ptr, filc_ptr tzp_ptr)
{
    filc_check_read_int(tp_ptr, sizeof(struct timeval), NULL);
    filc_check_read_int(tzp_ptr, sizeof(struct timezone), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(tp_ptr));
    filc_pin_tracked(my_thread, filc_ptr_object(tzp_ptr));
    filc_exit(my_thread);
    int result = settimeofday((const struct timeval*)filc_ptr_ptr(tp_ptr),
                              (const struct timezone*)filc_ptr_ptr(tzp_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_getrusage(filc_thread* my_thread, int who, filc_ptr rusage_ptr)
{
    filc_check_write_int(rusage_ptr, sizeof(struct rusage), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(rusage_ptr));
    filc_exit(my_thread);
    int result = getrusage(who, (struct rusage*)filc_ptr_ptr(rusage_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_flock(filc_thread* my_thread, int fd, int operation)
{
    filc_exit(my_thread);
    int result = flock(fd, operation);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_mkfifo(filc_thread* my_thread, filc_ptr path_ptr, unsigned short mode)
{
    char* path = filc_check_and_get_new_str(path_ptr);
    filc_exit(my_thread);
    int result = mkfifo(path, mode);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    bmalloc_deallocate(path);
    return result;
}

#endif /* PAS_ENABLE_FILC && FILC_FILBSD */

#endif /* LIBPAS_ENABLED */
