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

/* FIXME: Most of this is really a great implementation of the "not-musl" syscalls that make sense for
   all platforms. Some of it is FilBSD-specific. Would be great to eventually separate the two parts.

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
#include <ufs/ufs/quota.h>
#include <nfs/nfssvc.h>
#include <fs/nfs/nfsproto.h>
#include <fs/nfs/nfskpiport.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/nfsport.h>
#include <sys/timex.h>
#include <sys/timeffc.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/linker.h>
#include <sys/jail.h>
#include <sys/extattr.h>
#include <sys/event.h>
#include <sys/mac.h>
#include <sys/uio.h>
#include <sys/uuid.h>
#include <kenv.h>
#include <sys/regression.h>
#include <sys/_semaphore.h>
#include <bsm/audit.h>
#include <sys/mqueue.h>
#include <sys/umtx.h>

#define _ACL_PRIVATE 1
#include <sys/acl.h>
#undef _ACL_PRIVATE

static pas_lock roots_lock = PAS_LOCK_INITIALIZER;
static filc_object* profil_samples_root = NULL;
static filc_exact_ptr_table* kevent_ptr_table = NULL;

void filc_mark_user_global_roots(filc_object_array* mark_stack)
{
    pas_lock_lock(&roots_lock);
    fugc_mark(mark_stack, profil_samples_root);
    if (kevent_ptr_table)
        fugc_mark(mark_stack, filc_object_for_special_payload(kevent_ptr_table));
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
    int fd;
    unsigned long request;
    int result;
} ioctl_data;

static void ioctl_callback(void* guarded_arg, void* user_arg)
{
    ioctl_data* data = (ioctl_data*)user_arg;
    data->result = ioctl(data->fd, data->result, guarded_arg);
}

int filc_native_zsys_ioctl(filc_thread* my_thread, int fd, unsigned long request, filc_ptr args)
{
    if (filc_ptr_available(args) < FILC_WORD_SIZE)
        return FILC_SYSCALL(my_thread, ioctl(fd, request, NULL));

    filc_ptr arg_ptr = filc_ptr_get_next_ptr(my_thread, &args);
    if (!filc_ptr_ptr(arg_ptr))
        return FILC_SYSCALL(my_thread, ioctl(fd, request, NULL));

    ioctl_data data;
    data.fd = fd;
    data.request = request;
    filc_call_syscall_with_guarded_ptr(my_thread, arg_ptr, ioctl_callback, &data);
    return data.result;
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
    if (result < 0)
        filc_set_errno(my_errno);
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
    filc_check_write_int(pollfds_ptr, filc_mul_size(sizeof(struct pollfd), nfds), NULL);
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
    char* dir = filc_check_and_get_tmp_str(my_thread, dir_ptr);
    filc_exit(my_thread);
    int result = unmount(dir, flags);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

struct user_export_args {
	uint64_t ex_flags;		/* export related flags */
	uid_t	 ex_root;		/* mapping for root uid */
	uid_t	 ex_uid;		/* mapping for anonymous user */
	int	 ex_ngroups;
	filc_ptr ex_groups;
	filc_ptr ex_addr;	        /* net address to which exported */
	u_char	 ex_addrlen;		/* and the net address length */
	filc_ptr ex_mask;	        /* mask of valid bits in saddr */
	u_char	 ex_masklen;		/* and the smask length */
	filc_ptr ex_indexfile;		/* index file for WebNFS URLs */
	int	 ex_numsecflavors;	/* security flavor count */
	int	 ex_secflavors[MAXSECFLAVORS]; /* list of security flavors */
};

static void check_user_export_args(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_export_args, ex_flags, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_export_args, ex_root, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_export_args, ex_uid, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_export_args, ex_ngroups, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_export_args, ex_groups, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_export_args, ex_addr, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_export_args, ex_addrlen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_export_args, ex_mask, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_export_args, ex_masklen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_export_args, ex_indexfile, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_export_args, ex_numsecflavors, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_export_args, ex_secflavors, access_kind);
}

static void from_user_export_args(filc_thread* my_thread, struct user_export_args* user_export_args,
                                  struct export_args* export_args)
{
    export_args->ex_flags = user_export_args->ex_flags;
    export_args->ex_root = user_export_args->ex_root;
    export_args->ex_uid = user_export_args->ex_uid;
    export_args->ex_ngroups = user_export_args->ex_ngroups;
    filc_ptr groups_ptr = filc_ptr_load(my_thread, &user_export_args->ex_groups);
    filc_check_read_int(groups_ptr, filc_mul_size(sizeof(gid_t), export_args->ex_ngroups), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(groups_ptr));
    export_args->ex_groups = (gid_t*)filc_ptr_ptr(groups_ptr);
    export_args->ex_addrlen = user_export_args->ex_addrlen;
    filc_ptr addr_ptr = filc_ptr_load(my_thread, &user_export_args->ex_addr);
    filc_check_read_int(addr_ptr, export_args->ex_addrlen, NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(addr_ptr));
    export_args->ex_addr = (struct sockaddr*)filc_ptr_ptr(addr_ptr);
    export_args->ex_masklen = user_export_args->ex_masklen;
    filc_ptr mask_ptr = filc_ptr_load(my_thread, &user_export_args->ex_mask);
    filc_check_read_int(mask_ptr, export_args->ex_masklen, NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(mask_ptr));
    export_args->ex_mask = (struct sockaddr*)filc_ptr_ptr(mask_ptr);
    filc_ptr indexfile_ptr = filc_ptr_load(my_thread, &user_export_args->ex_indexfile);
    export_args->ex_indexfile = filc_check_and_get_tmp_str(my_thread, indexfile_ptr);
    export_args->ex_numsecflavors = user_export_args->ex_numsecflavors;
    PAS_ASSERT(sizeof(export_args->ex_secflavors) == sizeof(user_export_args->ex_secflavors));
    memcpy(export_args->ex_secflavors, user_export_args->ex_secflavors,
           sizeof(export_args->ex_secflavors));
}

struct user_xucred {
	u_int	cr_version;		/* structure layout version */
	uid_t	cr_uid;			/* effective user id */
	short	cr_ngroups;		/* number of groups */
	gid_t	cr_groups[XU_NGROUPS];	/* groups */
	union {
		filc_ptr _cr_unused1;	/* compatibility with old ucred */
		pid_t	cr_pid;
	};
};

static void check_user_xucred(filc_ptr ptr, filc_access_kind access_kind)
{
    filc_check_access_int(ptr, sizeof(struct user_xucred), access_kind, NULL);
}

static void from_user_xucred(struct user_xucred* user_xucred, struct xucred* xucred)
{
    xucred->cr_version = user_xucred->cr_version;
    xucred->cr_uid = user_xucred->cr_uid;
    xucred->cr_ngroups = user_xucred->cr_ngroups;
    PAS_ASSERT(sizeof(xucred->cr_groups) == sizeof(user_xucred->cr_groups));
    memcpy(xucred->cr_groups, user_xucred->cr_groups,
           sizeof(xucred->cr_groups));
    xucred->cr_pid = user_xucred->cr_pid;
}

struct user_o2export_args {
	uint64_t ex_flags;		/* export related flags */
	uid_t	 ex_root;		/* mapping for root uid */
	struct user_xucred ex_anon;     /* mapping for anonymous user */
	filc_ptr ex_addr;	        /* net address to which exported */
	u_char	 ex_addrlen;		/* and the net address length */
	filc_ptr ex_mask;	        /* mask of valid bits in saddr */
	u_char	 ex_masklen;		/* and the smask length */
	filc_ptr ex_indexfile;		/* index file for WebNFS URLs */
	int	 ex_numsecflavors;	/* security flavor count */
	int	 ex_secflavors[MAXSECFLAVORS]; /* list of security flavors */
};

static void check_user_o2export_args(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_o2export_args, ex_flags, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_o2export_args, ex_root, access_kind);
    check_user_xucred(filc_ptr_with_offset(ptr, PAS_OFFSETOF(struct user_o2export_args, ex_anon)),
                      access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_o2export_args, ex_addr, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_o2export_args, ex_addrlen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_o2export_args, ex_mask, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_o2export_args, ex_masklen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_o2export_args, ex_indexfile, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_o2export_args, ex_numsecflavors, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_o2export_args, ex_secflavors, access_kind);
}

static void from_user_o2export_args(filc_thread* my_thread,
                                    struct user_o2export_args* user_export_args,
                                    struct o2export_args* export_args)
{
    export_args->ex_flags = user_export_args->ex_flags;
    export_args->ex_root = user_export_args->ex_root;
    from_user_xucred(&user_export_args->ex_anon, &export_args->ex_anon);
    export_args->ex_addrlen = user_export_args->ex_addrlen;
    filc_ptr addr_ptr = filc_ptr_load(my_thread, &user_export_args->ex_addr);
    filc_check_read_int(addr_ptr, export_args->ex_addrlen, NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(addr_ptr));
    export_args->ex_addr = (struct sockaddr*)filc_ptr_ptr(addr_ptr);
    export_args->ex_masklen = user_export_args->ex_masklen;
    filc_ptr mask_ptr = filc_ptr_load(my_thread, &user_export_args->ex_mask);
    filc_check_read_int(mask_ptr, export_args->ex_masklen, NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(mask_ptr));
    export_args->ex_mask = (struct sockaddr*)filc_ptr_ptr(mask_ptr);
    filc_ptr indexfile_ptr = filc_ptr_load(my_thread, &user_export_args->ex_indexfile);
    export_args->ex_indexfile = filc_check_and_get_tmp_str(my_thread, indexfile_ptr);
    export_args->ex_numsecflavors = user_export_args->ex_numsecflavors;
    PAS_ASSERT(sizeof(export_args->ex_secflavors) == sizeof(user_export_args->ex_secflavors));
    memcpy(export_args->ex_secflavors, user_export_args->ex_secflavors,
           sizeof(export_args->ex_secflavors));
}

struct iovec* prepare_nmount_iovec(filc_thread* my_thread, filc_ptr user_iov, unsigned niov)
{
    struct iovec* iov;
    size_t index;
    FILC_CHECK(
        !(niov % 2),
        NULL,
        "niov must be multiple of 2; niov = %u.\n", niov);
    iov = filc_bmalloc_allocate_tmp(my_thread, filc_mul_size(niov, sizeof(struct iovec)));
    for (index = 0; index < (size_t)niov; index += 2) {
        filc_ptr key_iov_base;
        size_t key_iov_len;
        filc_extract_user_iovec_entry(
            my_thread, filc_ptr_with_offset(user_iov, filc_mul_size(sizeof(filc_user_iovec), index)),
            &key_iov_base, &key_iov_len);
        filc_check_read_int(key_iov_base, key_iov_len, NULL);
        char* key = filc_check_and_get_tmp_str_for_int_memory(
            my_thread, (char*)filc_ptr_ptr(key_iov_base), key_iov_len);
        iov[index].iov_base = key;
        iov[index].iov_len = key_iov_len;

        if (!strcmp(key, "errmsg")) {
            filc_prepare_iovec_entry(
                my_thread,
                filc_ptr_with_offset(user_iov, filc_mul_size(sizeof(filc_user_iovec), index + 1)),
                iov + index + 1, filc_write_access);
            continue;
        }

        if (!strcmp(key, "export")) {
            filc_ptr export_base;
            size_t export_len;
            filc_extract_user_iovec_entry(
                my_thread,
                filc_ptr_with_offset(user_iov, filc_mul_size(sizeof(filc_user_iovec), index + 1)),
                &export_base, &export_len);
            FILC_CHECK(
                export_len >= sizeof(struct user_export_args),
                NULL,
                "length of \"export\" argument is smaller than struct user_export_args.");
            check_user_export_args(export_base, filc_read_access);
            struct export_args* args =
                filc_bmalloc_allocate_tmp(my_thread, sizeof(struct export_args));
            from_user_export_args(my_thread, (struct user_export_args*)filc_ptr_ptr(export_base),
                                  args);
            iov[index + 1].iov_base = args;
            iov[index + 1].iov_len = sizeof(struct export_args);
            continue;
        }
        
        filc_prepare_iovec_entry(
            my_thread,
            filc_ptr_with_offset(user_iov, filc_mul_size(sizeof(filc_user_iovec), index + 1)),
            iov + index + 1, filc_read_access);
    }
    return iov;
}

int filc_native_zsys_nmount(filc_thread* my_thread, filc_ptr iov_ptr, unsigned niov, int flags)
{
    struct iovec* iov = prepare_nmount_iovec(my_thread, iov_ptr, niov);
    filc_exit(my_thread);
    int result = nmount(iov, niov, flags);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_chflags(filc_thread* my_thread, filc_ptr path_ptr, unsigned long flags)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, chflags(path, flags));
}

int filc_native_zsys_lchflags(filc_thread* my_thread, filc_ptr path_ptr, unsigned long flags)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, lchflags(path, flags));
}

int filc_native_zsys_fchflags(filc_thread* my_thread, int fd, unsigned long flags)
{
    return FILC_SYSCALL(my_thread, fchflags(fd, flags));
}

int filc_native_zsys_chflagsat(filc_thread* my_thread, int fd, filc_ptr path_ptr, unsigned long flags,
                               int atflag)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, chflagsat(fd, path, flags, atflag));
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
    char* name = filc_check_and_get_tmp_str(my_thread, name_ptr);
    filc_exit(my_thread);
    int result = setlogin(name);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_acct(filc_thread* my_thread, filc_ptr file_ptr)
{
    char* file = filc_check_and_get_tmp_str(my_thread, file_ptr);
    filc_exit(my_thread);
    int result = acct(file);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
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
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    filc_exit(my_thread);
    int result = revoke(path);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_ktrace(filc_thread* my_thread, filc_ptr tracefile_ptr, int ops, int trpoints,
                            int pid)
{
    char* tracefile = filc_check_and_get_tmp_str(my_thread, tracefile_ptr);
    filc_exit(my_thread);
    int result = ktrace(tracefile, ops, trpoints, pid);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
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
    char* special = filc_check_and_get_tmp_str(my_thread, special_ptr);
    filc_exit(my_thread);
    int result = swapon(special);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_swapoff(filc_thread* my_thread, filc_ptr special_ptr, unsigned flags)
{
    char* special = filc_check_and_get_tmp_str(my_thread, special_ptr);
    filc_exit(my_thread);
    int result = swapoff(special, flags);
    int my_errno = errno;
    filc_enter(my_thread);
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
    if (filc_ptr_ptr(tp_ptr)) {
        filc_check_write_int(tp_ptr, sizeof(struct timeval), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(tp_ptr));
    }
    if (filc_ptr_ptr(tzp_ptr)) {
        filc_check_write_int(tzp_ptr, sizeof(struct timezone), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(tzp_ptr));
    }
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
    if (filc_ptr_ptr(tp_ptr)) {
        filc_check_read_int(tp_ptr, sizeof(struct timeval), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(tp_ptr));
    }
    if (filc_ptr_ptr(tzp_ptr)) {
        filc_check_read_int(tzp_ptr, sizeof(struct timezone), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(tzp_ptr));
    }
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

static int utimes_impl(filc_thread* my_thread, filc_ptr path_ptr, filc_ptr times_ptr,
                       int (*actual_utimes)(const char*, const struct timeval*))
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    if (filc_ptr_ptr(times_ptr)) {
        filc_check_read_int(times_ptr, sizeof(struct timeval) * 2, NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(times_ptr));
    }
    filc_exit(my_thread);
    int result = actual_utimes(path, (const struct timeval*)filc_ptr_ptr(times_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_utimes(filc_thread* my_thread, filc_ptr path_ptr, filc_ptr times_ptr)
{
    return utimes_impl(my_thread, path_ptr, times_ptr, utimes);
}

int filc_native_zsys_lutimes(filc_thread* my_thread, filc_ptr path_ptr, filc_ptr times_ptr)
{
    return utimes_impl(my_thread, path_ptr, times_ptr, lutimes);
}

int filc_native_zsys_adjtime(filc_thread* my_thread, filc_ptr delta_ptr, filc_ptr olddelta_ptr)
{
    if (filc_ptr_ptr(delta_ptr)) {
        filc_check_read_int(delta_ptr, sizeof(struct timeval), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(delta_ptr));
    }
    if (filc_ptr_ptr(olddelta_ptr)) {
        filc_check_write_int(olddelta_ptr, sizeof(struct timeval), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(olddelta_ptr));
    }
    filc_exit(my_thread);
    int result = adjtime((const struct timeval*)filc_ptr_ptr(delta_ptr),
                         (struct timeval*)filc_ptr_ptr(olddelta_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_quotactl(filc_thread* my_thread, filc_ptr path_ptr, int cmd, int id,
                              filc_ptr addr_ptr)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    void* addr = NULL;
    bool pin_addr = false;
    switch (cmd >> SUBCMDSHIFT) {
    case Q_QUOTAON:
        addr = filc_check_and_get_tmp_str(my_thread, addr_ptr);
        break;
    case Q_GETQUOTASIZE:
        filc_check_write_int(addr_ptr, sizeof(int), NULL);
        pin_addr = true;
        break;
    case Q_GETQUOTA:
        filc_check_write_int(addr_ptr, sizeof(struct dqblk), NULL);
        pin_addr = true;
        break;
    case Q_SETQUOTA:
    case Q_SETUSE:
        filc_check_read_int(addr_ptr, sizeof(struct dqblk), NULL);
        pin_addr = true;
        break;
    default:
        break;
    }
    if (pin_addr) {
        filc_pin_tracked(my_thread, filc_ptr_object(addr_ptr));
        addr = filc_ptr_ptr(addr_ptr);
    }
    filc_exit(my_thread);
    int result = quotactl(path, cmd, id, addr);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_nlm_syscall(filc_thread* my_thread, int debug_level, int grace_period,
                                 int addr_count, filc_ptr addrs_ptr)
{
    size_t num_addrs = (size_t)addr_count;
    size_t total_size;
    FILC_CHECK(
        !pas_mul_uintptr_overflow(sizeof(char*), num_addrs, &total_size),
        NULL,
        "addr_count is so big that it caused overflow (addr_count = %d).", addr_count);
    char** addrs = filc_bmalloc_allocate_tmp(my_thread, total_size);
    size_t index;
    for (index = num_addrs; index--;) {
        filc_ptr addr_ptr_ptr = filc_ptr_with_offset(
            addrs_ptr, filc_mul_size(sizeof(filc_ptr), index));
        filc_check_read_ptr(addr_ptr_ptr, NULL);
        filc_ptr addr_ptr = filc_ptr_load(my_thread, (filc_ptr*)filc_ptr_ptr(addr_ptr_ptr));
        addrs[index] = filc_check_and_get_tmp_str(my_thread, addr_ptr);
    }
    filc_exit(my_thread);
    int result = nlm_syscall(debug_level, grace_period, addr_count, addrs);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

struct user_nfsd_idargs {
	int		nid_flag;	/* Flags (see below) */
	uid_t		nid_uid;	/* user/group id */
	gid_t		nid_gid;
	int		nid_usermax;	/* Upper bound on user name cache */
	int		nid_usertimeout;/* User name timeout (minutes) */
	filc_ptr	nid_name;	/* Name */
	int		nid_namelen;	/* and its length */
	filc_ptr	nid_grps;	/* and the list */
	int		nid_ngroup;	/* Size of groups list */
};

static void check_user_nfsd_idargs(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_idargs, nid_flag, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_idargs, nid_uid, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_idargs, nid_gid, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_idargs, nid_usermax, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_idargs, nid_usertimeout, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_idargs, nid_name, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_idargs, nid_namelen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_idargs, nid_grps, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_idargs, nid_ngroup, access_kind);
}

struct user_nfsd_oidargs {
	int		nid_flag;	/* Flags (see below) */
	uid_t		nid_uid;	/* user/group id */
	gid_t		nid_gid;
	int		nid_usermax;	/* Upper bound on user name cache */
	int		nid_usertimeout;/* User name timeout (minutes) */
	filc_ptr	nid_name;	/* Name */
	int		nid_namelen;	/* and its length */
};

static void check_user_nfsd_oidargs(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_oidargs, nid_flag, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_oidargs, nid_uid, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_oidargs, nid_gid, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_oidargs, nid_usermax, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_oidargs, nid_usertimeout, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_oidargs, nid_name, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_oidargs, nid_namelen, access_kind);
}

struct user_nfscbd_args {
	int	 sock;		/* Socket to serve */
	filc_ptr name;		/* Client addr for connection based sockets */
	int	 namelen;	/* Length of name */
	u_short	 port;		/* Port# for callbacks */
};

static void check_user_nfscbd_args(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_nfscbd_args, sock, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfscbd_args, name, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfscbd_args, namelen, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfscbd_args, port, access_kind);
}

struct user_nfscl_dumpmntopts {
	filc_ptr ndmnt_fname;		/* File Name */
	size_t	 ndmnt_blen;		/* Size of buffer */
	filc_ptr ndmnt_buf;		/* and the buffer */
};

static void check_user_nfscl_dumpmntopts(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfscl_dumpmntopts, ndmnt_fname, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfscl_dumpmntopts, ndmnt_blen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfscl_dumpmntopts, ndmnt_buf, access_kind);
}

struct user_nfsd_addsock_args {
	int	 sock;		/* Socket to serve */
	filc_ptr name;		/* Client addr for connection based sockets */
	int	 namelen;	/* Length of name */
};

static void check_user_nfsd_addsock_args(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_addsock_args, sock, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_addsock_args, name, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_addsock_args, namelen, access_kind);
}

struct user_nfsd_nfsd_args {
	filc_ptr principal;	/* GSS-API service principal name */
	int	 minthreads;	/* minimum service thread count */
	int	 maxthreads;	/* maximum service thread count */
	int	 version;	/* Allow multiple variants */
	filc_ptr addr;		/* pNFS DS addresses */
	int	 addrlen;	/* Length of addrs */
	filc_ptr dnshost;	/* DNS names for DS addresses */
	int	 dnshostlen;	/* Length of DNS names */
	filc_ptr dspath;	/* DS Mount path on MDS */
	int	 dspathlen;	/* Length of DS Mount path on MDS */
	filc_ptr mdspath;	/* MDS mount for DS path on MDS */
	int	 mdspathlen;	/* Length of MDS mount for DS path on MDS */
	int	 mirrorcnt;	/* Number of mirrors to create on DSs */
};

static void check_user_nfsd_nfsd_args(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_nfsd_args, principal, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_nfsd_args, minthreads, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_nfsd_args, maxthreads, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_nfsd_args, version, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_nfsd_args, addr, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_nfsd_args, addrlen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_nfsd_args, dnshost, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_nfsd_args, dnshostlen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_nfsd_args, dspath, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_nfsd_args, dspathlen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_nfsd_args, mdspath, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_nfsd_args, mdspathlen, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_nfsd_args, mirrorcnt, access_kind);
}

struct user_nfsd_nfsd_oargs {
	filc_ptr principal;	/* GSS-API service principal name */
	int	 minthreads;	/* minimum service thread count */
	int	 maxthreads;	/* maximum service thread count */
};

static void check_user_nfsd_nfsd_oargs(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_nfsd_oargs, principal, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_nfsd_oargs, minthreads, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_nfsd_oargs, maxthreads, access_kind);
}

struct user_nfsd_pnfsd_args {
	int	 op;		/* Which pNFSd op to perform. */
	filc_ptr mdspath;	/* Path of MDS file. */
	filc_ptr dspath;	/* Path of recovered DS mounted on dir. */
	filc_ptr curdspath;	/* Path of current DS mounted on dir. */
};

static void check_user_nfsd_pnfsd_args(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_pnfsd_args, op, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_pnfsd_args, mdspath, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_pnfsd_args, dspath, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_pnfsd_args, curdspath, access_kind);
}

struct user_nfsex_args {
	filc_ptr fspec;
	struct user_export_args	export;
};

static void check_user_nfsex_args(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsex_args, fspec, access_kind);
    check_user_export_args(
        filc_ptr_with_offset(ptr, PAS_OFFSETOF(struct user_nfsex_args, export)), access_kind);
}

struct user_nfsex_oldargs {
	filc_ptr fspec;
	struct user_o2export_args	export;
};

static void check_user_nfsex_oldargs(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsex_oldargs, fspec, access_kind);
    check_user_o2export_args(
        filc_ptr_with_offset(ptr, PAS_OFFSETOF(struct user_nfsex_oldargs, export)), access_kind);
}

struct user_nfsd_dumplist {
	int		ndl_size;	/* Number of elements */
	filc_ptr        ndl_list;	/* and the list of elements */
};

static void check_user_nfsd_dumplist(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_dumplist, ndl_size, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_dumplist, ndl_list, access_kind);
}

struct user_nfsd_dumplocklist {
	filc_ptr        ndllck_fname;	/* File Name */
	int		ndllck_size;	/* Number of elements */
	filc_ptr        ndllck_list;	/* and the list of elements */
};

static void check_user_nfsd_dumplocklist(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_dumplocklist, ndllck_fname, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_nfsd_dumplocklist, ndllck_size, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_nfsd_dumplocklist, ndllck_list, access_kind);
}

static int nfssvc_impl(filc_thread* my_thread, int flags, void* argstructp)
{
    filc_exit(my_thread);
    int result = nfssvc(flags, argstructp);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_nfssvc(filc_thread* my_thread, int flags, filc_ptr argstructp_ptr)
{
    if ((flags & NFSSVC_IDNAME)) {
        if ((flags & NFSSVC_NEWSTRUCT)) {
            struct nfsd_idargs args;
            check_user_nfsd_idargs(argstructp_ptr, filc_read_access);
            struct user_nfsd_idargs* user_args =
                (struct user_nfsd_idargs*)filc_ptr_ptr(argstructp_ptr);
            args.nid_flag = user_args->nid_flag;
            args.nid_uid = user_args->nid_uid;
            args.nid_gid = user_args->nid_gid;
            args.nid_usermax = user_args->nid_usermax;
            args.nid_usertimeout = user_args->nid_usertimeout;
            filc_ptr name_ptr = filc_ptr_load(my_thread, &user_args->nid_name);
            int namelen = user_args->nid_namelen;
            filc_check_read_int(name_ptr, namelen, NULL);
            filc_pin_tracked(my_thread, filc_ptr_object(name_ptr));
            args.nid_name = (u_char*)filc_ptr_ptr(name_ptr);
            args.nid_namelen = namelen;
            filc_ptr grps_ptr = filc_ptr_load(my_thread, &user_args->nid_grps);
            int ngroup = user_args->nid_ngroup;
            filc_check_read_int(grps_ptr, filc_mul_size(ngroup, sizeof(gid_t)), NULL);
            filc_pin_tracked(my_thread, filc_ptr_object(grps_ptr));
            args.nid_grps = (gid_t*)filc_ptr_ptr(grps_ptr);
            args.nid_ngroup = ngroup;
            return nfssvc_impl(my_thread, flags, &args);
        }
        struct nfsd_oidargs args;
        check_user_nfsd_oidargs(argstructp_ptr, filc_read_access);
        struct user_nfsd_oidargs* user_args =
            (struct user_nfsd_oidargs*)filc_ptr_ptr(argstructp_ptr);
        args.nid_flag = user_args->nid_flag;
        args.nid_uid = user_args->nid_uid;
        args.nid_gid = user_args->nid_gid;
        args.nid_usermax = user_args->nid_usermax;
        args.nid_usertimeout = user_args->nid_usertimeout;
        filc_ptr name_ptr = filc_ptr_load(my_thread, &user_args->nid_name);
        int namelen = user_args->nid_namelen;
        filc_check_read_int(name_ptr, namelen, NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(name_ptr));
        args.nid_name = (u_char*)filc_ptr_ptr(name_ptr);
        args.nid_namelen = namelen;
        return nfssvc_impl(my_thread, flags, &args);
    }
    if ((flags & NFSSVC_GETSTATS)) {
        if ((flags & NFSSVC_NEWSTRUCT)) {
            filc_check_read_int(argstructp_ptr, sizeof(int), NULL);
            int version = *(int*)filc_ptr_ptr(argstructp_ptr);
            if (version == NFSSTATS_V1) {
                filc_check_write_int(argstructp_ptr, sizeof(struct nfsstatsv1), NULL);
                filc_pin_tracked(my_thread, filc_ptr_object(argstructp_ptr));
                return nfssvc_impl(my_thread, flags, filc_ptr_ptr(argstructp_ptr));
            }
            if (version == NFSSTATS_OV1) {
                filc_check_write_int(argstructp_ptr, sizeof(struct nfsstatsov1), NULL);
                filc_pin_tracked(my_thread, filc_ptr_object(argstructp_ptr));
                return nfssvc_impl(my_thread, flags, filc_ptr_ptr(argstructp_ptr));
            }
            filc_set_errno(EPERM);
            return -1;
        }
        filc_check_write_int(argstructp_ptr, sizeof(struct ext_nfsstats), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(argstructp_ptr));
        return nfssvc_impl(my_thread, flags, filc_ptr_ptr(argstructp_ptr));
    }
    if ((flags & NFSSVC_NFSUSERDPORT)) {
        if ((flags & NFSSVC_NEWSTRUCT)) {
            filc_check_read_int(argstructp_ptr, sizeof(struct nfsuserd_args), NULL);
            filc_pin_tracked(my_thread, filc_ptr_object(argstructp_ptr));
            return nfssvc_impl(my_thread, flags, filc_ptr_ptr(argstructp_ptr));
        }
        filc_check_read_int(argstructp_ptr, sizeof(u_short), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(argstructp_ptr));
        return nfssvc_impl(my_thread, flags, filc_ptr_ptr(argstructp_ptr));
    }
    if ((flags & NFSSVC_NFSUSERDDELPORT) ||
        (flags & NFSSVC_NOPUBLICFH) ||
        (flags & NFSSVC_BACKUPSTABLE) ||
        (flags & NFSSVC_SUSPENDNFSD) ||
        (flags & NFSSVC_RESUMENFSD))
        return nfssvc_impl(my_thread, flags, NULL);
    if ((flags & NFSSVC_CBADDSOCK)) {
        struct nfscbd_args args;
        check_user_nfscbd_args(argstructp_ptr, filc_read_access);
        struct user_nfscbd_args* user_args = (struct user_nfscbd_args*)filc_ptr_ptr(argstructp_ptr);
        pas_zero_memory(&args, sizeof(args));
        args.sock = user_args->sock;
        args.port = user_args->port;
        /* The name/namelen are not used by the kernel, and set to NULL/0 by the user, so we ignore
           it! */
        return nfssvc_impl(my_thread, flags, &args);
    }
    if ((flags & NFSSVC_NFSCBD)) {
        filc_check_read_ptr(argstructp_ptr, NULL);
        filc_ptr str_ptr = filc_ptr_load(my_thread, (filc_ptr*)filc_ptr_ptr(argstructp_ptr));
        char* str = filc_check_and_get_tmp_str(my_thread, str_ptr);
        return nfssvc_impl(my_thread, flags, &str);
    }
    if ((flags & NFSSVC_DUMPMNTOPTS)) {
        struct nfscl_dumpmntopts args;
        check_user_nfscl_dumpmntopts(argstructp_ptr, filc_read_access);
        struct user_nfscl_dumpmntopts* user_args =
            (struct user_nfscl_dumpmntopts*)filc_ptr_ptr(argstructp_ptr);
        filc_ptr fname_ptr = filc_ptr_load(my_thread, &user_args->ndmnt_fname);
        args.ndmnt_fname = filc_check_and_get_tmp_str(my_thread, fname_ptr);
        args.ndmnt_blen = user_args->ndmnt_blen;
        filc_ptr buf_ptr = filc_ptr_load(my_thread, &user_args->ndmnt_buf);
        filc_check_write_int(buf_ptr, args.ndmnt_blen, NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(buf_ptr));
        args.ndmnt_buf = filc_ptr_ptr(buf_ptr);
        return nfssvc_impl(my_thread, flags, &args);
    }
    if ((flags & NFSSVC_FORCEDISM)) {
        char* buf = filc_check_and_get_tmp_str(my_thread, argstructp_ptr);
        return nfssvc_impl(my_thread, flags, buf);
    }
    if ((flags & NFSSVC_NFSDADDSOCK)) {
        struct nfsd_addsock_args args;
        check_user_nfsd_addsock_args(argstructp_ptr, filc_read_access);
        struct user_nfsd_addsock_args* user_args =
            (struct user_nfsd_addsock_args*)filc_ptr_ptr(argstructp_ptr);
        pas_zero_memory(&args, sizeof(args));
        args.sock = user_args->sock;
        /* The name/namelen are not used by the kernel, and set to NULL/0 by the user, so we ignore
           it! */
        return nfssvc_impl(my_thread, flags, &args);
    }
    if ((flags & NFSSVC_NFSDNFSD)) {
        if ((flags & NFSSVC_NEWSTRUCT)) {
            struct nfsd_nfsd_args args;
            check_user_nfsd_nfsd_args(argstructp_ptr, filc_read_access);
            struct user_nfsd_nfsd_args* user_args =
                (struct user_nfsd_nfsd_args*)filc_ptr_ptr(argstructp_ptr);
            filc_ptr principal_ptr = filc_ptr_load(my_thread, &user_args->principal);
            args.principal = filc_check_and_get_tmp_str(my_thread, principal_ptr);
            args.minthreads = user_args->minthreads;
            args.maxthreads = user_args->maxthreads;
            args.version = user_args->version;
            args.addrlen = user_args->addrlen;
            filc_ptr addr_ptr = filc_ptr_load(my_thread, &user_args->addr);
            filc_check_read_int(addr_ptr, args.addrlen, NULL);
            filc_pin_tracked(my_thread, filc_ptr_object(addr_ptr));
            args.addr = (char*)filc_ptr_ptr(addr_ptr);
            args.dnshostlen = user_args->dnshostlen;
            filc_ptr dnshost_ptr = filc_ptr_load(my_thread, &user_args->dnshost);
            filc_check_read_int(dnshost_ptr, args.dnshostlen, NULL);
            filc_pin_tracked(my_thread, filc_ptr_object(dnshost_ptr));
            args.dnshost = (char*)filc_ptr_ptr(dnshost_ptr);
            args.dspathlen = user_args->dspathlen;
            filc_ptr dspath_ptr = filc_ptr_load(my_thread, &user_args->dspath);
            filc_check_read_int(dspath_ptr, args.dspathlen, NULL);
            filc_pin_tracked(my_thread, filc_ptr_object(dspath_ptr));
            args.dspath = (char*)filc_ptr_ptr(dspath_ptr);
            args.mdspathlen = user_args->mdspathlen;
            filc_ptr mdspath_ptr = filc_ptr_load(my_thread, &user_args->mdspath);
            filc_check_read_int(mdspath_ptr, args.mdspathlen, NULL);
            filc_pin_tracked(my_thread, filc_ptr_object(mdspath_ptr));
            args.mdspath = (char*)filc_ptr_ptr(mdspath_ptr);
            args.mirrorcnt = user_args->mirrorcnt;
            return nfssvc_impl(my_thread, flags, &args);
        }
        struct nfsd_nfsd_oargs args;
        check_user_nfsd_nfsd_oargs(argstructp_ptr, filc_read_access);
        struct user_nfsd_nfsd_oargs* user_args =
            (struct user_nfsd_nfsd_oargs*)filc_ptr_ptr(argstructp_ptr);
        filc_ptr principal_ptr = filc_ptr_load(my_thread, &user_args->principal);
        args.principal = filc_check_and_get_tmp_str(my_thread, principal_ptr);
        args.minthreads = user_args->minthreads;
        args.maxthreads = user_args->maxthreads;
        return nfssvc_impl(my_thread, flags, &args);
    }
    if ((flags & NFSSVC_PNFSDS)) {
        struct nfsd_pnfsd_args args;
        check_user_nfsd_pnfsd_args(argstructp_ptr, filc_read_access);
        struct user_nfsd_pnfsd_args* user_args =
            (struct user_nfsd_pnfsd_args*)filc_ptr_ptr(argstructp_ptr);
        args.op = user_args->op;
        filc_ptr mdspath_ptr = filc_ptr_load(my_thread, &user_args->mdspath);
        args.mdspath = filc_check_and_get_tmp_str(my_thread, mdspath_ptr);
        filc_ptr dspath_ptr = filc_ptr_load(my_thread, &user_args->dspath);
        args.dspath = filc_check_and_get_tmp_str(my_thread, dspath_ptr);
        filc_ptr curdspath_ptr = filc_ptr_load(my_thread, &user_args->curdspath);
        args.curdspath = filc_check_and_get_tmp_str(my_thread, curdspath_ptr);
        return nfssvc_impl(my_thread, flags, &args);
    }
    if ((flags & NFSSVC_PUBLICFH)) {
        filc_check_read_int(argstructp_ptr, sizeof(fhandle_t), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(argstructp_ptr));
        return nfssvc_impl(my_thread, flags, filc_ptr_ptr(argstructp_ptr));
    }
    if ((flags & NFSSVC_V4ROOTEXPORT)) {
        if ((flags & NFSSVC_NEWSTRUCT)) {
            struct nfsex_args args;
            check_user_nfsex_args(argstructp_ptr, filc_read_access);
            struct user_nfsex_args* user_args = (struct user_nfsex_args*)filc_ptr_ptr(argstructp_ptr);
            filc_ptr fspec_ptr = filc_ptr_load(my_thread, &user_args->fspec);
            args.fspec = filc_check_and_get_tmp_str(my_thread, fspec_ptr);
            from_user_export_args(my_thread, &user_args->export, &args.export);
            return nfssvc_impl(my_thread, flags, &args);
        }
        struct nfsex_oldargs args;
        check_user_nfsex_oldargs(argstructp_ptr, filc_read_access);
        struct user_nfsex_oldargs* user_args =
            (struct user_nfsex_oldargs*)filc_ptr_ptr(argstructp_ptr);
        filc_ptr fspec_ptr = filc_ptr_load(my_thread, &user_args->fspec);
        args.fspec = filc_check_and_get_tmp_str(my_thread, fspec_ptr);
        from_user_o2export_args(my_thread, &user_args->export, &args.export);
        return nfssvc_impl(my_thread, flags, &args);
    }
    if ((flags & NFSSVC_STABLERESTART)) {
        filc_check_read_int(argstructp_ptr, sizeof(int), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(argstructp_ptr));
        return nfssvc_impl(my_thread, flags, filc_ptr_ptr(argstructp_ptr));
    }
    if ((flags & NFSSVC_ADMINREVOKE)) {
        filc_check_read_int(argstructp_ptr, sizeof(struct nfsd_clid), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(argstructp_ptr));
        return nfssvc_impl(my_thread, flags, filc_ptr_ptr(argstructp_ptr));
    }
    if ((flags & NFSSVC_DUMPCLIENTS)) {
        struct nfsd_dumplist args;
        check_user_nfsd_dumplist(argstructp_ptr, filc_read_access);
        struct user_nfsd_dumplist* user_args =
            (struct user_nfsd_dumplist*)filc_ptr_ptr(argstructp_ptr);
        args.ndl_size = user_args->ndl_size;
        filc_ptr list_ptr = filc_ptr_load(my_thread, &user_args->ndl_list);
        filc_check_write_int(list_ptr, filc_mul_size(args.ndl_size, sizeof(struct nfsd_dumpclients)),
                             NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(list_ptr));
        args.ndl_list = filc_ptr_ptr(list_ptr);
        return nfssvc_impl(my_thread, flags, &args);
    }
    if ((flags & NFSSVC_DUMPLOCKS)) {
        struct nfsd_dumplocklist args;
        check_user_nfsd_dumplocklist(argstructp_ptr, filc_read_access);
        struct user_nfsd_dumplocklist* user_args =
            (struct user_nfsd_dumplocklist*)filc_ptr_ptr(argstructp_ptr);
        args.ndllck_fname = filc_check_and_get_tmp_str(
            my_thread, filc_ptr_load(my_thread, &user_args->ndllck_fname));
        args.ndllck_size = user_args->ndllck_size;
        filc_ptr list_ptr = filc_ptr_load(my_thread, &user_args->ndllck_list);
        filc_check_write_int(list_ptr, filc_mul_size(args.ndllck_size, sizeof(struct nfsd_dumplocks)),
                             NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(list_ptr));
        args.ndllck_list = filc_ptr_ptr(list_ptr);
        return nfssvc_impl(my_thread, flags, &args);
    }
    filc_set_errno(EINVAL);
    return -1;
}

static int rtprio_impl(filc_thread* my_thread, int function, int pidish, filc_ptr rtp_ptr,
                       int (*actual_rtprio)(int, int, struct rtprio*))
{
    switch (function) {
    case RTP_LOOKUP:
        filc_check_write_int(rtp_ptr, sizeof(struct rtprio), NULL);
        break;
    case RTP_SET:
        filc_check_read_int(rtp_ptr, sizeof(struct rtprio), NULL);
        break;
    default:
        filc_set_errno(EINVAL);
        return -1;
    }
    filc_pin_tracked(my_thread, filc_ptr_object(rtp_ptr));
    filc_exit(my_thread);
    int result = actual_rtprio(function, pidish, (struct rtprio*)filc_ptr_ptr(rtp_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_rtprio(filc_thread* my_thread, int function, int pid, filc_ptr rtp_ptr)
{
    return rtprio_impl(my_thread, function, pid, rtp_ptr, rtprio);
}

int filc_native_zsys_rtprio_thread(filc_thread* my_thread, int function, int pid, filc_ptr rtp_ptr)
{
    return rtprio_impl(my_thread, function, pid, rtp_ptr, rtprio_thread);
}

static int getfh_impl(filc_thread* my_thread, filc_ptr path_ptr, filc_ptr fhp_ptr,
                      int (*actual_getfh)(const char*, fhandle_t*))
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    filc_check_write_int(fhp_ptr, sizeof(fhandle_t), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(fhp_ptr));
    filc_exit(my_thread);
    int result = actual_getfh(path, (fhandle_t*)filc_ptr_ptr(fhp_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_getfh(filc_thread* my_thread, filc_ptr path_ptr, filc_ptr fhp_ptr)
{
    return getfh_impl(my_thread, path_ptr, fhp_ptr, getfh);
}

int filc_native_zsys_lgetfh(filc_thread* my_thread, filc_ptr path_ptr, filc_ptr fhp_ptr)
{
    return getfh_impl(my_thread, path_ptr, fhp_ptr, lgetfh);
}

int filc_native_zsys_getfhat(filc_thread* my_thread, int fd, filc_ptr path_ptr, filc_ptr fhp_ptr,
                             int flags)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    filc_check_write_int(fhp_ptr, sizeof(fhandle_t), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(fhp_ptr));
    filc_exit(my_thread);
    int result = getfhat(fd, path, (fhandle_t*)filc_ptr_ptr(fhp_ptr), flags);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_setfib(filc_thread* my_thread, int fib)
{
    filc_exit(my_thread);
    int result = setfib(fib);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return 0;
}

int filc_native_zsys_ntp_adjtime(filc_thread* my_thread, filc_ptr timex_ptr)
{
    filc_check_write_int(timex_ptr, sizeof(struct timex), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(timex_ptr));
    filc_exit(my_thread);
    int result = ntp_adjtime((struct timex*)filc_ptr_ptr(timex_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_ntp_gettime(filc_thread* my_thread, filc_ptr timeval_ptr)
{
    filc_check_write_int(timeval_ptr, sizeof(struct ntptimeval), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(timeval_ptr));
    filc_exit(my_thread);
    int result = ntp_gettime((struct ntptimeval*)filc_ptr_ptr(timeval_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

static long pathconf_impl(filc_thread* my_thread, filc_ptr path_ptr, int name,
                          long (*actual_pathconf)(const char*, int))
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    filc_exit(my_thread);
    errno = 0;
    long result = actual_pathconf(path, name);
    int my_errno = errno;
    filc_enter(my_thread);
    if (my_errno)
        filc_set_errno(my_errno);
    return result;
}

long filc_native_zsys_pathconf(filc_thread* my_thread, filc_ptr path_ptr, int name)
{
    return pathconf_impl(my_thread, path_ptr, name, pathconf);
}

long filc_native_zsys_lpathconf(filc_thread* my_thread, filc_ptr path_ptr, int name)
{
    return pathconf_impl(my_thread, path_ptr, name, lpathconf);
}

long filc_native_zsys_fpathconf(filc_thread* my_thread, int fd, int name)
{
    filc_exit(my_thread);
    errno = 0;
    long result = fpathconf(fd, name);
    int my_errno = errno;
    filc_enter(my_thread);
    if (my_errno)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_setrlimit(filc_thread* my_thread, int resource, filc_ptr rlp_ptr)
{
    filc_check_read_int(rlp_ptr, sizeof(struct rlimit), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(rlp_ptr));
    filc_exit(my_thread);
    int result = setrlimit(resource, (struct rlimit*)filc_ptr_ptr(rlp_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int __sysctl(const int *name, unsigned namelen, void *oldp, size_t *oldlenp, const void *newp,
             size_t newlen);

int filc_native_zsys___sysctl(filc_thread* my_thread, filc_ptr name_ptr, unsigned namelen,
                              filc_ptr oldp_ptr, filc_ptr oldlenp_ptr, filc_ptr newp_ptr,
                              size_t newlen)
{
    filc_check_read_int(name_ptr, filc_mul_size(namelen, sizeof(int)), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(name_ptr));
    if (filc_ptr_ptr(oldp_ptr) || filc_ptr_ptr(oldlenp_ptr)) {
        filc_check_write_int(oldlenp_ptr, sizeof(size_t), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(oldlenp_ptr));
    }
    if (filc_ptr_ptr(oldp_ptr)) {
        filc_check_write_int(oldp_ptr, *(size_t*)filc_ptr_ptr(oldlenp_ptr), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(oldp_ptr));
    }
    if (filc_ptr_ptr(newp_ptr)) {
        filc_check_read_int(newp_ptr, newlen, NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(newp_ptr));
    }
    filc_exit(my_thread);
    int result = __sysctl((int*)filc_ptr_ptr(name_ptr), namelen, filc_ptr_ptr(oldp_ptr),
                          (size_t*)filc_ptr_ptr(oldlenp_ptr), filc_ptr_ptr(newp_ptr), newlen);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_undelete(filc_thread* my_thread, filc_ptr path_ptr)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    filc_exit(my_thread);
    int result = undelete(path);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

/* It's totally a goal to implement SysV IPC, including the shared memory parts. I just don't have to,
   yet. */
int filc_native_zsys_semget(filc_thread* my_thread, long key, int nsems, int flag)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(key);
    PAS_UNUSED_PARAM(nsems);
    PAS_UNUSED_PARAM(flag);
    filc_internal_panic(NULL, "zsys_semget not implemented.");
    return -1;
}

int filc_native_zsys_semctl(filc_thread* my_thread, int semid, int semnum, int cmd, filc_ptr args)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(semid);
    PAS_UNUSED_PARAM(semnum);
    PAS_UNUSED_PARAM(cmd);
    PAS_UNUSED_PARAM(args);
    filc_internal_panic(NULL, "zsys_semctl not implemented.");
    return -1;
}

int filc_native_zsys_semop(filc_thread* my_thread, int semid, filc_ptr array_ptr, size_t nops)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(semid);
    PAS_UNUSED_PARAM(array_ptr);
    PAS_UNUSED_PARAM(nops);
    filc_internal_panic(NULL, "zsys_semop not implemented.");
    return -1;
}

int filc_native_zsys_shmget(filc_thread* my_thread, long key, size_t size, int flag)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(key);
    PAS_UNUSED_PARAM(size);
    PAS_UNUSED_PARAM(flag);
    filc_internal_panic(NULL, "zsys_shmget not implemented.");
    return -1;
}

int filc_native_zsys_shmctl(filc_thread* my_thread, int shmid, int cmd, filc_ptr buf_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(shmid);
    PAS_UNUSED_PARAM(cmd);
    PAS_UNUSED_PARAM(buf_ptr);
    filc_internal_panic(NULL, "zsys_shmctl not implemented.");
    return -1;
}

filc_ptr filc_native_zsys_shmat(filc_thread* my_thread, int shmid, filc_ptr addr_ptr, int flag)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(shmid);
    PAS_UNUSED_PARAM(addr_ptr);
    PAS_UNUSED_PARAM(flag);
    filc_internal_panic(NULL, "zsys_shmat not implemented.");
    return filc_ptr_forge_null();
}

int filc_native_zsys_shmdt(filc_thread* my_thread, filc_ptr addr_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(addr_ptr);
    filc_internal_panic(NULL, "zsys_shmdt not implemented.");
    return -1;
}

int filc_native_zsys_msgget(filc_thread* my_thread, long key, int msgflg)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(key);
    PAS_UNUSED_PARAM(msgflg);
    filc_internal_panic(NULL, "zsys_msgget not implemented.");
    return -1;
}

int filc_native_zsys_msgctl(filc_thread* my_thread, int msgid, int cmd, filc_ptr buf_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(msgid);
    PAS_UNUSED_PARAM(cmd);
    PAS_UNUSED_PARAM(buf_ptr);
    filc_internal_panic(NULL, "zsys_msgctl not implemented.");
    return -1;
}

long filc_native_zsys_msgrcv(filc_thread* my_thread, int msgid, filc_ptr msgp_ptr, size_t msgsz,
                             long msgtyp, int msgflg)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(msgid);
    PAS_UNUSED_PARAM(msgp_ptr);
    PAS_UNUSED_PARAM(msgsz);
    PAS_UNUSED_PARAM(msgtyp);
    PAS_UNUSED_PARAM(msgflg);
    filc_internal_panic(NULL, "zsys_msgrcv not implemented.");
    return -1;
}

int filc_native_zsys_msgsnd(filc_thread* my_thread, int msgid, filc_ptr msgp_ptr, size_t msgsz,
                            int msgflg)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(msgid);
    PAS_UNUSED_PARAM(msgp_ptr);
    PAS_UNUSED_PARAM(msgsz);
    PAS_UNUSED_PARAM(msgflg);
    filc_internal_panic(NULL, "zsys_msgsnd not implemented.");
    return -1;
}

int filc_native_zsys_futimes(filc_thread* my_thread, int fd, filc_ptr times_ptr)
{
    if (filc_ptr_ptr(times_ptr)) {
        filc_check_read_int(times_ptr, sizeof(struct timeval) * 2, NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(times_ptr));
    }
    filc_exit(my_thread);
    return FILC_SYSCALL(my_thread, futimes(fd, (const struct timeval*)filc_ptr_ptr(times_ptr)));
}

int filc_native_zsys_futimesat(filc_thread* my_thread, int fd, filc_ptr path_ptr, filc_ptr times_ptr)
{
    if (filc_ptr_ptr(times_ptr)) {
        filc_check_read_int(times_ptr, sizeof(struct timeval) * 2, NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(times_ptr));
    }
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, futimesat(fd, path,
                                             (const struct timeval*)filc_ptr_ptr(times_ptr)));
}

int ktimer_create(clockid_t, struct sigevent *__restrict, int *__restrict);

int filc_native_zsys_ktimer_create(filc_thread* my_thread, int clock, filc_ptr sigev_ptr,
                                   filc_ptr oshandle_ptr)
{
    if (filc_ptr_ptr(sigev_ptr)) {
        filc_check_read_int(sigev_ptr, sizeof(struct sigevent), NULL);
        filc_pin_tracked(my_thread, filc_ptr_object(sigev_ptr));
    }
    filc_check_write_int(oshandle_ptr, sizeof(int), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(oshandle_ptr));
    filc_exit(my_thread);
    int result = ktimer_create(clock, (struct sigevent*)filc_ptr_ptr(sigev_ptr),
                               (int*)filc_ptr_ptr(oshandle_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_clock_settime(filc_thread* my_thread, int clock_id, filc_ptr tp_ptr)
{
    filc_check_read_int(tp_ptr, sizeof(struct timespec), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(tp_ptr));
    filc_exit(my_thread);
    int result = clock_settime(clock_id, (struct timespec*)filc_ptr_ptr(tp_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_clock_getres(filc_thread* my_thread, int clock_id, filc_ptr tp_ptr)
{
    filc_check_write_int(tp_ptr, sizeof(struct timespec), NULL);
    filc_pin_tracked(my_thread, filc_ptr_object(tp_ptr));
    filc_exit(my_thread);
    int result = clock_getres(clock_id, (struct timespec*)filc_ptr_ptr(tp_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int ktimer_delete(int oshandle);

int filc_native_zsys_ktimer_delete(filc_thread* my_thread, int oshandle)
{
    filc_exit(my_thread);
    int result = ktimer_delete(oshandle);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int ktimer_gettime(int oshandle, struct itimerspec* itimerp);

int filc_native_zsys_ktimer_gettime(filc_thread* my_thread, int oshandle, filc_ptr itimerp_ptr)
{
    filc_cpt_write_int(my_thread, itimerp_ptr, sizeof(struct itimerspec));
    return FILC_SYSCALL(
        my_thread, ktimer_gettime(oshandle, (struct itimerspec*)filc_ptr_ptr(itimerp_ptr)));
}

int ktimer_getoverrun(int oshandle);

int filc_native_zsys_ktimer_getoverrun(filc_thread* my_thread, int oshandle)
{
    return FILC_SYSCALL(my_thread, ktimer_getoverrun(oshandle));
}

int ktimer_settime(int oshandle, int flags, const struct itimerspec* new_value,
                   struct itimerspec* old_value);

int filc_native_zsys_ktimer_settime(filc_thread* my_thread, int oshandle, int flags,
                                    filc_ptr new_value_ptr, filc_ptr old_value_ptr)
{
    if (filc_ptr_ptr(new_value_ptr))
        filc_cpt_read_int(my_thread, new_value_ptr, sizeof(struct itimerspec));
    if (filc_ptr_ptr(old_value_ptr))
        filc_cpt_write_int(my_thread, old_value_ptr, sizeof(struct itimerspec));
    return FILC_SYSCALL(
        my_thread, ktimer_settime(oshandle, flags,
                                  (const struct itimerspec*)filc_ptr_ptr(new_value_ptr),
                                  (struct itimerspec*)filc_ptr_ptr(old_value_ptr)));
}

int filc_native_zsys_ffclock_getcounter(filc_thread* my_thread, filc_ptr ffcount_ptr)
{
    filc_cpt_write_int(my_thread, ffcount_ptr, sizeof(ffcounter));
    return FILC_SYSCALL(my_thread, ffclock_getcounter((ffcounter*)filc_ptr_ptr(ffcount_ptr)));
}

int filc_native_zsys_ffclock_getestimate(filc_thread* my_thread, filc_ptr cest_ptr)
{
    filc_cpt_write_int(my_thread, cest_ptr, sizeof(struct ffclock_estimate));
    return FILC_SYSCALL(
        my_thread, ffclock_getestimate((struct ffclock_estimate*)filc_ptr_ptr(cest_ptr)));
}

int filc_native_zsys_ffclock_setestimate(filc_thread* my_thread, filc_ptr cest_ptr)
{
    filc_cpt_write_int(my_thread, cest_ptr, sizeof(struct ffclock_estimate));
    return FILC_SYSCALL(
        my_thread, ffclock_setestimate((struct ffclock_estimate*)filc_ptr_ptr(cest_ptr)));
}

int filc_native_zsys_clock_getcpuclockid2(filc_thread* my_thread, long long id, int which,
                                          filc_ptr clock_id_ptr)
{
    filc_cpt_write_int(my_thread, clock_id_ptr, sizeof(clockid_t));
    return FILC_SYSCALL(
        my_thread, clock_getcpuclockid2(id, which, (clockid_t*)filc_ptr_ptr(clock_id_ptr)));
}

int filc_native_zsys_minherit(filc_thread* my_thread, filc_ptr addr_ptr, size_t len, int inherit)
{
    switch (inherit) {
    case INHERIT_SHARE:
        filc_check_write_int(addr_ptr, len, NULL);
        break;
    case INHERIT_COPY:
    case INHERIT_ZERO:
        filc_check_access_common(addr_ptr, len, filc_write_access, NULL);
        break;
    case INHERIT_NONE:
        filc_internal_panic(NULL, "cannot minherit INHERIT_NONE, no safe implementation yet.");
        break;
    default:
        filc_set_errno(EINVAL);
        return -1;
    }
    filc_check_pin_and_track_mmap(my_thread, addr_ptr);
    return FILC_SYSCALL(my_thread, minherit(filc_ptr_ptr(addr_ptr), len, inherit));
}

int filc_native_zsys_issetugid(filc_thread* my_thread)
{
    filc_exit(my_thread);
    int result = issetugid();
    filc_enter(my_thread);
    return result;
}

int filc_native_zsys_aio_read(filc_thread* my_thread, filc_ptr iocb_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(iocb_ptr);
    filc_internal_panic(NULL, "zsys_aio_read not implemented.");
    return -1;
}

int filc_native_zsys_aio_readv(filc_thread* my_thread, filc_ptr iocb_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(iocb_ptr);
    filc_internal_panic(NULL, "zsys_aio_readv not implemented.");
    return -1;
}

int filc_native_zsys_aio_write(filc_thread* my_thread, filc_ptr iocb_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(iocb_ptr);
    filc_internal_panic(NULL, "zsys_aio_write not implemented.");
    return -1;
}

int filc_native_zsys_aio_writev(filc_thread* my_thread, filc_ptr iocb_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(iocb_ptr);
    filc_internal_panic(NULL, "zsys_aio_writev not implemented.");
    return -1;
}

int filc_native_zsys_lio_listio(filc_thread* my_thread, int mode, filc_ptr list_ptr, int niocb,
                                filc_ptr timeout_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(mode);
    PAS_UNUSED_PARAM(list_ptr);
    PAS_UNUSED_PARAM(niocb);
    PAS_UNUSED_PARAM(timeout_ptr);
    filc_internal_panic(NULL, "zsys_lio_listio not implemented.");
    return -1;
}

int filc_native_zsys_aio_return(filc_thread* my_thread, filc_ptr iocb_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(iocb_ptr);
    filc_internal_panic(NULL, "zsys_aio_return not implemented.");
    return -1;
}

int filc_native_zsys_aio_suspend(filc_thread* my_thread, filc_ptr iocbs_ptr, int niocb,
                                 filc_ptr timeout_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(iocbs_ptr);
    PAS_UNUSED_PARAM(niocb);
    PAS_UNUSED_PARAM(timeout_ptr);
    filc_internal_panic(NULL, "zsys_aio_suspend not implemented.");
    return -1;
}

int filc_native_zsys_aio_error(filc_thread* my_thread, filc_ptr iocb_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(iocb_ptr);
    filc_internal_panic(NULL, "zsys_aio_error not implemented.");
    return -1;
}

long filc_native_zsys_aio_waitcomplete(filc_thread* my_thread, filc_ptr iocbp_ptr,
                                       filc_ptr timeout_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(iocbp_ptr);
    PAS_UNUSED_PARAM(timeout_ptr);
    filc_internal_panic(NULL, "zsys_aio_waitcomplete not implemented.");
    return -1;
}

int filc_native_zsys_aio_fsync(filc_thread* my_thread, int op, filc_ptr iocb_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(op);
    PAS_UNUSED_PARAM(iocb_ptr);
    filc_internal_panic(NULL, "zsys_aio_fsync not implemented.");
    return -1;
}

int filc_native_zsys_aio_mlock(filc_thread* my_thread, filc_ptr iocb_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(iocb_ptr);
    filc_internal_panic(NULL, "zsys_aio_mlock not implemented.");
    return -1;
}

int filc_native_zsys_fhopen(filc_thread* my_thread, filc_ptr fhp_ptr, int flags)
{
    filc_cpt_read_int(my_thread, fhp_ptr, sizeof(fhandle_t));
    return FILC_SYSCALL(my_thread, fhopen((const fhandle_t*)filc_ptr_ptr(fhp_ptr), flags));
}

int filc_native_zsys_fhstat(filc_thread* my_thread, filc_ptr fhp_ptr, filc_ptr sb_ptr)
{
    filc_cpt_read_int(my_thread, fhp_ptr, sizeof(fhandle_t));
    filc_cpt_write_int(my_thread, sb_ptr, sizeof(struct stat));
    return FILC_SYSCALL(my_thread, fhstat((const fhandle_t*)filc_ptr_ptr(fhp_ptr),
                                          (struct stat*)filc_ptr_ptr(sb_ptr)));
}

int filc_native_zsys_fhstatfs(filc_thread* my_thread, filc_ptr fhp_ptr, filc_ptr buf_ptr)
{
    filc_cpt_read_int(my_thread, fhp_ptr, sizeof(fhandle_t));
    filc_cpt_write_int(my_thread, buf_ptr, sizeof(struct statfs));
    return FILC_SYSCALL(my_thread, fhstatfs((const fhandle_t*)filc_ptr_ptr(fhp_ptr),
                                            (struct statfs*)filc_ptr_ptr(buf_ptr)));
}

int filc_native_zsys_modnext(filc_thread* my_thread, int modid)
{
    return FILC_SYSCALL(my_thread, modnext(modid));
}

int filc_native_zsys_modfnext(filc_thread* my_thread, int modid)
{
    return FILC_SYSCALL(my_thread, modfnext(modid));
}

int filc_native_zsys_modstat(filc_thread* my_thread, int modid, filc_ptr stat_ptr)
{
    filc_cpt_write_int(my_thread, stat_ptr, sizeof(struct module_stat));
    return FILC_SYSCALL(my_thread, modstat(modid, (struct module_stat*)filc_ptr_ptr(stat_ptr)));
}

int filc_native_zsys_modfind(filc_thread* my_thread, filc_ptr modname_ptr)
{
    char* modname = filc_check_and_get_tmp_str(my_thread, modname_ptr);
    return FILC_SYSCALL(my_thread, modfind(modname));
}

int filc_native_zsys_kldload(filc_thread* my_thread, filc_ptr file_ptr)
{
    char* file = filc_check_and_get_tmp_str(my_thread, file_ptr);
    return FILC_SYSCALL(my_thread, kldload(file));
}

int filc_native_zsys_kldunload(filc_thread* my_thread, int fileid)
{
    return FILC_SYSCALL(my_thread, kldunload(fileid));
}

int filc_native_zsys_kldunloadf(filc_thread* my_thread, int fileid, int flags)
{
    return FILC_SYSCALL(my_thread, kldunloadf(fileid, flags));
}

int filc_native_zsys_kldfind(filc_thread* my_thread, filc_ptr file_ptr)
{
    char* file = filc_check_and_get_tmp_str(my_thread, file_ptr);
    return FILC_SYSCALL(my_thread, kldfind(file));
}

int filc_native_zsys_kldnext(filc_thread* my_thread, int fileid)
{
    return FILC_SYSCALL(my_thread, kldnext(fileid));
}

int filc_native_zsys_getresgid(filc_thread* my_thread, filc_ptr rgid_ptr, filc_ptr egid_ptr,
                               filc_ptr sgid_ptr)
{
    filc_cpt_write_int(my_thread, rgid_ptr, sizeof(gid_t));
    filc_cpt_write_int(my_thread, egid_ptr, sizeof(gid_t));
    filc_cpt_write_int(my_thread, sgid_ptr, sizeof(gid_t));
    return FILC_SYSCALL(my_thread, getresgid((gid_t*)filc_ptr_ptr(rgid_ptr),
                                             (gid_t*)filc_ptr_ptr(egid_ptr),
                                             (gid_t*)filc_ptr_ptr(sgid_ptr)));
}

int filc_native_zsys_getresuid(filc_thread* my_thread, filc_ptr ruid_ptr, filc_ptr euid_ptr,
                               filc_ptr suid_ptr)
{
    filc_cpt_write_int(my_thread, ruid_ptr, sizeof(uid_t));
    filc_cpt_write_int(my_thread, euid_ptr, sizeof(uid_t));
    filc_cpt_write_int(my_thread, suid_ptr, sizeof(uid_t));
    return FILC_SYSCALL(my_thread, getresuid((uid_t*)filc_ptr_ptr(ruid_ptr),
                                             (uid_t*)filc_ptr_ptr(euid_ptr),
                                             (uid_t*)filc_ptr_ptr(suid_ptr)));
}

int filc_native_zsys_setresgid(filc_thread* my_thread, unsigned rgid, unsigned egid, unsigned sgid)
{
    return FILC_SYSCALL(my_thread, setresgid(rgid, egid, sgid));
}

int filc_native_zsys_setresuid(filc_thread* my_thread, unsigned ruid, unsigned euid, unsigned suid)
{
    return FILC_SYSCALL(my_thread, setresuid(ruid, euid, suid));
}

int filc_native_zsys_kldfirstmod(filc_thread* my_thread, int fileid)
{
    return FILC_SYSCALL(my_thread, kldfirstmod(fileid));
}

int filc_native_zsys_kldstat(filc_thread* my_thread, int fileid, filc_ptr stat_ptr)
{
    filc_cpt_write_int(my_thread, stat_ptr, sizeof(struct kld_file_stat));
    return FILC_SYSCALL(my_thread, kldstat(fileid, (struct kld_file_stat*)filc_ptr_ptr(stat_ptr)));
}

int __getcwd(char* buf, size_t size);

int filc_native_zsys___getcwd(filc_thread* my_thread, filc_ptr buf_ptr, size_t size)
{
    filc_cpt_write_int(my_thread, buf_ptr, size);
    return FILC_SYSCALL(my_thread, __getcwd((char*)filc_ptr_ptr(buf_ptr), size));
}

int filc_native_zsys_sched_setparam(filc_thread* my_thread, int pid, filc_ptr param_buf)
{
    filc_cpt_read_int(my_thread, param_buf, sizeof(struct sched_param));
    return FILC_SYSCALL(
        my_thread, sched_setparam(pid, (const struct sched_param*)filc_ptr_ptr(param_buf)));
}

int filc_native_zsys_sched_getparam(filc_thread* my_thread, int pid, filc_ptr param_buf)
{
    filc_cpt_write_int(my_thread, param_buf, sizeof(struct sched_param));
    return FILC_SYSCALL(my_thread, sched_getparam(pid, (struct sched_param*)filc_ptr_ptr(param_buf)));
}

int filc_native_zsys_sched_setscheduler(filc_thread* my_thread, int pid, int policy,
                                        filc_ptr param_buf)
{
    filc_cpt_read_int(my_thread, param_buf, sizeof(struct sched_param));
    return FILC_SYSCALL(
        my_thread, sched_setscheduler(pid, policy,
                                      (const struct sched_param*)filc_ptr_ptr(param_buf)));
}

int filc_native_zsys_sched_getscheduler(filc_thread* my_thread, int pid)
{
    return FILC_SYSCALL(my_thread, sched_getscheduler(pid));
}

int filc_native_zsys_sched_get_priority_min(filc_thread* my_thread, int policy)
{
    return FILC_SYSCALL(my_thread, sched_get_priority_min(policy));
}

int filc_native_zsys_sched_get_priority_max(filc_thread* my_thread, int policy)
{
    return FILC_SYSCALL(my_thread, sched_get_priority_max(policy));
}

int filc_native_zsys_sched_rr_get_interval(filc_thread* my_thread, int pid, filc_ptr interval_ptr)
{
    filc_cpt_write_int(my_thread, interval_ptr, sizeof(struct timespec));
    return FILC_SYSCALL(
        my_thread, sched_rr_get_interval(pid, (struct timespec*)filc_ptr_ptr(interval_ptr)));
}

int filc_native_zsys_utrace(filc_thread* my_thread, filc_ptr addr_ptr, size_t len)
{
    filc_cpt_read_int(my_thread, addr_ptr, len);
    return FILC_SYSCALL(my_thread, utrace(filc_ptr_ptr(addr_ptr), len));
}

struct user_kld_sym_lookup {
    int		version;	/* set to sizeof(struct kld_sym_lookup) */
    filc_ptr    symname;	/* Symbol name we are looking up */
    u_long	symvalue;
    size_t	symsize;
};

static void check_user_kld_sym_lookup(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_kld_sym_lookup, version, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_kld_sym_lookup, symname, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_kld_sym_lookup, symvalue, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_kld_sym_lookup, symsize, access_kind);
}

int filc_native_zsys_kldsym(filc_thread* my_thread, int fileid, int cmd, filc_ptr data_ptr)
{
    check_user_kld_sym_lookup(data_ptr, filc_read_access);
    struct kld_sym_lookup args;
    struct user_kld_sym_lookup* user_args = (struct user_kld_sym_lookup*)filc_ptr_ptr(data_ptr);
    pas_zero_memory(&args, sizeof(args));
    args.version = user_args->version;
    args.symname = filc_check_and_get_tmp_str(
        my_thread, filc_ptr_load(my_thread, &user_args->symname));
    int result = FILC_SYSCALL(my_thread, kldsym(fileid, cmd, &args));
    if (!result) {
        check_user_kld_sym_lookup(data_ptr, filc_write_access);
        user_args->symvalue = args.symvalue;
        user_args->symsize = args.symsize;
    }
    return result;
}

struct user_jail {
	uint32_t	version;
	filc_ptr        path;
	filc_ptr        hostname;
	filc_ptr        jailname;
	uint32_t	ip4s;
	uint32_t	ip6s;
	filc_ptr        ip4;
	filc_ptr        ip6;
};

static void check_user_jail(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_jail, version, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_jail, path, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_jail, hostname, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_jail, jailname, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_jail, ip4s, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_jail, ip6s, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_jail, ip4, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_jail, ip6, access_kind);
}

int filc_native_zsys_jail(filc_thread* my_thread, filc_ptr jail_ptr)
{
    check_user_jail(jail_ptr, filc_read_access);
    struct jail args;
    struct user_jail* user_args = (struct user_jail*)filc_ptr_ptr(jail_ptr);
    pas_zero_memory(&args, sizeof(args));
    args.version = user_args->version;
    args.path = filc_check_and_get_tmp_str(my_thread, filc_ptr_load(my_thread, &user_args->path));
    args.hostname = filc_check_and_get_tmp_str(
        my_thread, filc_ptr_load(my_thread, &user_args->hostname));
    args.jailname = filc_check_and_get_tmp_str(
        my_thread, filc_ptr_load(my_thread, &user_args->jailname));
    args.ip4s = user_args->ip4s;
    args.ip6s = user_args->ip6s;
    filc_ptr ip4_ptr = filc_ptr_load(my_thread, &user_args->ip4);
    filc_cpt_read_int(my_thread, ip4_ptr, filc_mul_size(sizeof(struct in_addr), args.ip4s));
    args.ip4 = (struct in_addr*)filc_ptr_ptr(ip4_ptr);
    filc_ptr ip6_ptr = filc_ptr_load(my_thread, &user_args->ip6);
    filc_cpt_read_int(my_thread, ip6_ptr, filc_mul_size(sizeof(struct in6_addr), args.ip6s));
    args.ip6 = (struct in6_addr*)filc_ptr_ptr(ip6_ptr);
    return FILC_SYSCALL(my_thread, jail(&args));
}

int filc_native_zsys_jail_attach(filc_thread* my_thread, int jid)
{
    return FILC_SYSCALL(my_thread, jail_attach(jid));
}

int filc_native_zsys_jail_remove(filc_thread* my_thread, int jid)
{
    return FILC_SYSCALL(my_thread, jail_remove(jid));
}

static struct iovec* prepare_key_value_query_iovec(filc_thread* my_thread, filc_ptr iov_ptr,
                                                   unsigned niov)
{
    struct iovec* iov;
    size_t index;
    FILC_CHECK(
        !(niov % 2),
        NULL,
        "niov must be even (niov = %u).", niov);
    iov = filc_bmalloc_allocate_tmp(my_thread, filc_mul_size(sizeof(struct iovec), niov));
    for (index = 0; index < niov; index += 2) {
        filc_prepare_iovec_entry(
            my_thread, filc_ptr_with_offset(iov_ptr, filc_mul_size(sizeof(filc_user_iovec), index)),
            iov + index, filc_read_access);
        filc_prepare_iovec_entry(
            my_thread,
            filc_ptr_with_offset(iov_ptr, filc_mul_size(sizeof(filc_user_iovec), index + 1)),
            iov + index + 1, filc_write_access);
    }
    return iov;
}

int filc_native_zsys_jail_get(filc_thread* my_thread, filc_ptr iov_ptr, unsigned niov, int flags)
{
    struct iovec* iov = prepare_key_value_query_iovec(my_thread, iov_ptr, niov);
    return FILC_SYSCALL(my_thread, jail_get(iov, niov, flags));
}

int filc_native_zsys_jail_set(filc_thread* my_thread, filc_ptr iov_ptr, unsigned niov, int flags)
{
    struct iovec* iov = filc_prepare_iovec(my_thread, iov_ptr, (int)niov, filc_read_access);
    return FILC_SYSCALL(my_thread, jail_set(iov, niov, flags));
}

int filc_native_zsys___acl_aclcheck_fd(filc_thread* my_thread, int fd, int type, filc_ptr aclp_ptr)
{
    filc_cpt_write_int(my_thread, aclp_ptr, sizeof(struct acl));
    return FILC_SYSCALL(my_thread, __acl_aclcheck_fd(fd, type, (struct acl*)filc_ptr_ptr(aclp_ptr)));
}

int filc_native_zsys___acl_aclcheck_file(filc_thread* my_thread, filc_ptr path_ptr, int type,
                                         filc_ptr aclp_ptr)
{
    filc_cpt_write_int(my_thread, aclp_ptr, sizeof(struct acl));
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(
        my_thread, __acl_aclcheck_file(path, type, (struct acl*)filc_ptr_ptr(aclp_ptr)));
}

int filc_native_zsys___acl_aclcheck_link(filc_thread* my_thread, filc_ptr path_ptr, int type,
                                         filc_ptr aclp_ptr)
{
    filc_cpt_write_int(my_thread, aclp_ptr, sizeof(struct acl));
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(
        my_thread, __acl_aclcheck_link(path, type, (struct acl*)filc_ptr_ptr(aclp_ptr)));
}

int filc_native_zsys___acl_delete_fd(filc_thread* my_thread, int fd, int type)
{
    return FILC_SYSCALL(my_thread, __acl_delete_fd(fd, type));
}

int filc_native_zsys___acl_delete_file(filc_thread* my_thread, filc_ptr path_ptr, int type)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, __acl_delete_file(path, type));
}

int filc_native_zsys___acl_delete_link(filc_thread* my_thread, filc_ptr path_ptr, int type)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, __acl_delete_link(path, type));
}

int filc_native_zsys___acl_get_fd(filc_thread* my_thread, int fd, int type, filc_ptr aclp_ptr)
{
    filc_cpt_write_int(my_thread, aclp_ptr, sizeof(struct acl));
    return FILC_SYSCALL(my_thread, __acl_get_fd(fd, type, (struct acl*)filc_ptr_ptr(aclp_ptr)));
}

int filc_native_zsys___acl_get_file(filc_thread* my_thread, filc_ptr path_ptr, int type,
                                    filc_ptr aclp_ptr)
{
    filc_cpt_write_int(my_thread, aclp_ptr, sizeof(struct acl));
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, __acl_get_file(path, type, (struct acl*)filc_ptr_ptr(aclp_ptr)));
}

int filc_native_zsys___acl_get_link(filc_thread* my_thread, filc_ptr path_ptr, int type,
                                    filc_ptr aclp_ptr)
{
    filc_cpt_write_int(my_thread, aclp_ptr, sizeof(struct acl));
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, __acl_get_link(path, type, (struct acl*)filc_ptr_ptr(aclp_ptr)));
}

int filc_native_zsys___acl_set_fd(filc_thread* my_thread, int fd, int type, filc_ptr aclp_ptr)
{
    filc_cpt_write_int(my_thread, aclp_ptr, sizeof(struct acl));
    return FILC_SYSCALL(my_thread, __acl_set_fd(fd, type, (struct acl*)filc_ptr_ptr(aclp_ptr)));
}

int filc_native_zsys___acl_set_file(filc_thread* my_thread, filc_ptr path_ptr, int type,
                                    filc_ptr aclp_ptr)
{
    filc_cpt_write_int(my_thread, aclp_ptr, sizeof(struct acl));
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, __acl_set_file(path, type, (struct acl*)filc_ptr_ptr(aclp_ptr)));
}

int filc_native_zsys___acl_set_link(filc_thread* my_thread, filc_ptr path_ptr, int type,
                                    filc_ptr aclp_ptr)
{
    filc_cpt_write_int(my_thread, aclp_ptr, sizeof(struct acl));
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, __acl_set_link(path, type, (struct acl*)filc_ptr_ptr(aclp_ptr)));
}

int filc_native_zsys_extattr_delete_fd(filc_thread* my_thread, int fd, int attrnamespace,
                                       filc_ptr attrname_ptr)
{
    char* attrname = filc_check_and_get_tmp_str(my_thread, attrname_ptr);
    return FILC_SYSCALL(my_thread, extattr_delete_fd(fd, attrnamespace, attrname));
}

int filc_native_zsys_extattr_delete_file(filc_thread* my_thread, filc_ptr path_ptr, int attrnamespace,
                                         filc_ptr attrname_ptr)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    char* attrname = filc_check_and_get_tmp_str(my_thread, attrname_ptr);
    return FILC_SYSCALL(my_thread, extattr_delete_file(path, attrnamespace, attrname));
}

int filc_native_zsys_extattr_delete_link(filc_thread* my_thread, filc_ptr path_ptr, int attrnamespace,
                                         filc_ptr attrname_ptr)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    char* attrname = filc_check_and_get_tmp_str(my_thread, attrname_ptr);
    return FILC_SYSCALL(my_thread, extattr_delete_link(path, attrnamespace, attrname));
}

long filc_native_zsys_extattr_get_fd(filc_thread* my_thread, int fd, int attrnamespace,
                                     filc_ptr attrname_ptr, filc_ptr data_ptr, size_t nbytes)
{
    char* attrname = filc_check_and_get_tmp_str(my_thread, attrname_ptr);
    filc_cpt_write_int(my_thread, data_ptr, nbytes);
    return FILC_SYSCALL(my_thread, extattr_get_fd(fd, attrnamespace, attrname, filc_ptr_ptr(data_ptr),
                                                  nbytes));
}

long filc_native_zsys_extattr_get_file(filc_thread* my_thread, filc_ptr path_ptr, int attrnamespace,
                                       filc_ptr attrname_ptr, filc_ptr data_ptr, size_t nbytes)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    char* attrname = filc_check_and_get_tmp_str(my_thread, attrname_ptr);
    filc_cpt_write_int(my_thread, data_ptr, nbytes);
    return FILC_SYSCALL(my_thread, extattr_get_file(path, attrnamespace, attrname,
                                                    filc_ptr_ptr(data_ptr), nbytes));
}

long filc_native_zsys_extattr_get_link(filc_thread* my_thread, filc_ptr path_ptr, int attrnamespace,
                                       filc_ptr attrname_ptr, filc_ptr data_ptr, size_t nbytes)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    char* attrname = filc_check_and_get_tmp_str(my_thread, attrname_ptr);
    filc_cpt_write_int(my_thread, data_ptr, nbytes);
    return FILC_SYSCALL(my_thread, extattr_get_link(path, attrnamespace, attrname,
                                                    filc_ptr_ptr(data_ptr), nbytes));
}

long filc_native_zsys_extattr_list_fd(filc_thread* my_thread, int fd, int attrnamespace,
                                      filc_ptr data_ptr, size_t nbytes)
{
    filc_cpt_write_int(my_thread, data_ptr, nbytes);
    return FILC_SYSCALL(my_thread, extattr_list_fd(fd, attrnamespace, filc_ptr_ptr(data_ptr),
                                                   nbytes));
}

long filc_native_zsys_extattr_list_file(filc_thread* my_thread, filc_ptr path_ptr, int attrnamespace,
                                        filc_ptr data_ptr, size_t nbytes)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    filc_cpt_write_int(my_thread, data_ptr, nbytes);
    return FILC_SYSCALL(my_thread, extattr_list_file(path, attrnamespace, filc_ptr_ptr(data_ptr),
                                                     nbytes));
}

long filc_native_zsys_extattr_list_link(filc_thread* my_thread, filc_ptr path_ptr, int attrnamespace,
                                        filc_ptr data_ptr, size_t nbytes)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    filc_cpt_write_int(my_thread, data_ptr, nbytes);
    return FILC_SYSCALL(my_thread, extattr_list_link(path, attrnamespace, filc_ptr_ptr(data_ptr),
                                                     nbytes));
}

long filc_native_zsys_extattr_set_fd(filc_thread* my_thread, int fd, int attrnamespace,
                                     filc_ptr attrname_ptr, filc_ptr data_ptr, size_t nbytes)
{
    char* attrname = filc_check_and_get_tmp_str(my_thread, attrname_ptr);
    filc_cpt_read_int(my_thread, data_ptr, nbytes);
    return FILC_SYSCALL(my_thread, extattr_set_fd(fd, attrnamespace, attrname, filc_ptr_ptr(data_ptr),
                                                  nbytes));
}

long filc_native_zsys_extattr_set_file(filc_thread* my_thread, filc_ptr path_ptr, int attrnamespace,
                                       filc_ptr attrname_ptr, filc_ptr data_ptr, size_t nbytes)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    char* attrname = filc_check_and_get_tmp_str(my_thread, attrname_ptr);
    filc_cpt_read_int(my_thread, data_ptr, nbytes);
    return FILC_SYSCALL(my_thread, extattr_set_file(path, attrnamespace, attrname,
                                                    filc_ptr_ptr(data_ptr), nbytes));
}

long filc_native_zsys_extattr_set_link(filc_thread* my_thread, filc_ptr path_ptr, int attrnamespace,
                                       filc_ptr attrname_ptr, filc_ptr data_ptr, size_t nbytes)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    char* attrname = filc_check_and_get_tmp_str(my_thread, attrname_ptr);
    filc_cpt_read_int(my_thread, data_ptr, nbytes);
    return FILC_SYSCALL(my_thread, extattr_set_link(path, attrnamespace, attrname,
                                                    filc_ptr_ptr(data_ptr), nbytes));
}

int filc_native_zsys_extattrctl(filc_thread* my_thread, filc_ptr path_ptr, int cmd,
                                filc_ptr filename_ptr, int attrnamespace, filc_ptr attrname_ptr)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    char* filename = filc_check_and_get_tmp_str(my_thread, filename_ptr);
    char* attrname = filc_check_and_get_tmp_str(my_thread, attrname_ptr);
    return FILC_SYSCALL(my_thread, extattrctl(path, cmd, filename, attrnamespace, attrname));
}

static filc_exact_ptr_table* get_kevent_ptr_table(filc_thread* my_thread)
{
    filc_exact_ptr_table* result = kevent_ptr_table;
    pas_fence();
    if (result)
        return result;
    result = filc_exact_ptr_table_create(my_thread);
    pas_fence();
    pas_lock_lock(&roots_lock);
    if (kevent_ptr_table)
        result = kevent_ptr_table;
    else {
        /* Don't need a filc_store_barrier because the ptr table is black. */
        kevent_ptr_table = result;
    }
    pas_lock_unlock(&roots_lock);
    return result;
}

static void* encode_kevent_udata(filc_thread* my_thread, filc_ptr udata_ptr)
{
    return (void*)filc_exact_ptr_table_encode(my_thread, get_kevent_ptr_table(my_thread), udata_ptr);
}

static filc_ptr decode_kevent_udata(filc_thread* my_thread, void* udata)
{
    return filc_exact_ptr_table_decode(my_thread, get_kevent_ptr_table(my_thread), (uintptr_t)udata);
}

struct user_kevent {
	__uintptr_t	ident;		/* identifier for this event */
	short		filter;		/* filter for event */
	unsigned short	flags;		/* action flags for kqueue */
	unsigned int	fflags;		/* filter flag value */
	__int64_t	data;		/* filter data value */
	filc_ptr        udata;		/* opaque user data identifier */
	__uint64_t	ext[4];		/* extensions */
};

static void check_user_kevent(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_kevent, ident, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_kevent, filter, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_kevent, flags, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_kevent, fflags, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_kevent, data, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_kevent, udata, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_kevent, ext, access_kind);
}

int filc_native_zsys_kqueue(filc_thread* my_thread)
{
    return FILC_SYSCALL(my_thread, kqueue());
}

int filc_native_zsys_kqueuex(filc_thread* my_thread, unsigned flags)
{
    return FILC_SYSCALL(my_thread, kqueuex(flags));
}

int filc_native_zsys_kevent(filc_thread* my_thread, int kq, filc_ptr changelist_ptr, int nchanges,
                            filc_ptr eventlist_ptr, int nevents, filc_ptr timeout_ptr)
{
    size_t index;
    struct kevent* changelist = (struct kevent*)filc_bmalloc_allocate_tmp(
        my_thread, filc_mul_size(nchanges, sizeof(struct kevent)));
    for (index = 0; index < (size_t)nchanges; ++index) {
        filc_ptr entry_ptr =
            filc_ptr_with_offset(changelist_ptr, filc_mul_size(index, sizeof(struct user_kevent)));
        check_user_kevent(entry_ptr, filc_read_access);
        struct kevent* entry = changelist + index;
        struct user_kevent* user_entry = (struct user_kevent*)filc_ptr_ptr(entry_ptr);
        entry->ident = user_entry->ident;
        entry->filter = user_entry->filter;
        entry->flags = user_entry->flags;
        entry->fflags = user_entry->fflags;
        entry->data = user_entry->data;
        entry->udata = encode_kevent_udata(my_thread, user_entry->udata);
        PAS_ASSERT(sizeof(entry->ext) == sizeof(user_entry->ext));
        memcpy(entry->ext, user_entry->ext, sizeof(entry->ext));
    }
    filc_cpt_read_int(my_thread, timeout_ptr, sizeof(struct timespec));
    struct kevent* eventlist = (struct kevent*)filc_bmalloc_allocate_tmp(
        my_thread, filc_mul_size(nevents, sizeof(struct kevent)));
    int result = FILC_SYSCALL(
        my_thread,
        kevent(kq, changelist, nchanges, eventlist, nevents,
               (const struct timespec*)filc_ptr_ptr(timeout_ptr)));
    if (result < 0)
        return result;
    PAS_ASSERT(result <= nevents);
    for (index = 0; index < (size_t)result; ++index) {
        filc_ptr change_ptr =
            filc_ptr_with_offset(eventlist_ptr, filc_mul_size(index, sizeof(struct user_kevent)));
        check_user_kevent(change_ptr, filc_write_access);
        struct kevent* change = eventlist + index;
        struct user_kevent* user_change = (struct user_kevent*)filc_ptr_ptr(change_ptr);
        user_change->ident = change->ident;
        user_change->filter = change->filter;
        user_change->flags = change->flags;
        user_change->fflags = change->fflags;
        user_change->data = change->data;
        user_change->udata = decode_kevent_udata(my_thread, change->udata);
        PAS_ASSERT(sizeof(change->ext) == sizeof(user_change->ext));
        memcpy(user_change->ext, change->ext, sizeof(change->ext));
    }
    return result;
}

struct user_mac {
    size_t m_buflen;
    filc_ptr m_string;
};

static void check_user_mac(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct user_mac, m_buflen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_mac, m_string, access_kind);
}

static struct mac* from_user_mac(filc_thread* my_thread, filc_ptr mac_ptr,
                                 filc_access_kind access_kind)
{
    check_user_mac(mac_ptr, filc_read_access);
    struct user_mac* user_mac = (struct user_mac*)filc_ptr_ptr(mac_ptr);
    size_t buflen = user_mac->m_buflen;
    filc_ptr string_ptr = filc_ptr_load(my_thread, &user_mac->m_string);
    filc_cpt_access_int(my_thread, string_ptr, buflen, access_kind);
    struct mac* result = filc_bmalloc_allocate_tmp(my_thread, sizeof(struct mac));
    result->m_buflen = buflen;
    result->m_string = (char*)filc_ptr_ptr(string_ptr);
    return result;
}

int filc_native_zsys___mac_get_fd(filc_thread* my_thread, int fd, filc_ptr mac_ptr)
{
    struct mac* mac = from_user_mac(my_thread, mac_ptr, filc_write_access);
    return FILC_SYSCALL(my_thread, mac_get_fd(fd, mac));
}

int filc_native_zsys___mac_get_file(filc_thread* my_thread, filc_ptr path_ptr, filc_ptr mac_ptr)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    struct mac* mac = from_user_mac(my_thread, mac_ptr, filc_write_access);
    return FILC_SYSCALL(my_thread, mac_get_file(path, mac));
}

int filc_native_zsys___mac_get_link(filc_thread* my_thread, filc_ptr path_ptr, filc_ptr mac_ptr)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    struct mac* mac = from_user_mac(my_thread, mac_ptr, filc_write_access);
    return FILC_SYSCALL(my_thread, mac_get_file(path, mac));
}

int filc_native_zsys___mac_get_pid(filc_thread* my_thread, int pid, filc_ptr mac_ptr)
{
    struct mac* mac = from_user_mac(my_thread, mac_ptr, filc_write_access);
    return FILC_SYSCALL(my_thread, mac_get_pid(pid, mac));
}

int filc_native_zsys___mac_get_proc(filc_thread* my_thread, filc_ptr mac_ptr)
{
    struct mac* mac = from_user_mac(my_thread, mac_ptr, filc_write_access);
    return FILC_SYSCALL(my_thread, mac_get_proc(mac));
}

int filc_native_zsys___mac_set_fd(filc_thread* my_thread, int fd, filc_ptr mac_ptr)
{
    struct mac* mac = from_user_mac(my_thread, mac_ptr, filc_read_access);
    return FILC_SYSCALL(my_thread, mac_set_fd(fd, mac));
}

int filc_native_zsys___mac_set_file(filc_thread* my_thread, filc_ptr path_ptr, filc_ptr mac_ptr)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    struct mac* mac = from_user_mac(my_thread, mac_ptr, filc_read_access);
    return FILC_SYSCALL(my_thread, mac_set_file(path, mac));
}

int filc_native_zsys___mac_set_link(filc_thread* my_thread, filc_ptr path_ptr, filc_ptr mac_ptr)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    struct mac* mac = from_user_mac(my_thread, mac_ptr, filc_read_access);
    return FILC_SYSCALL(my_thread, mac_set_file(path, mac));
}

int filc_native_zsys___mac_set_proc(filc_thread* my_thread, filc_ptr mac_ptr)
{
    struct mac* mac = from_user_mac(my_thread, mac_ptr, filc_read_access);
    return FILC_SYSCALL(my_thread, mac_set_proc(mac));
}

int filc_native_zsys___mac_execve(filc_thread* my_thread, filc_ptr fname_ptr, filc_ptr argv_ptr,
                                  filc_ptr env_ptr, filc_ptr mac_ptr)
{
    char* fname = filc_check_and_get_tmp_str(my_thread, fname_ptr);
    char** argv = filc_check_and_get_null_terminated_string_array(my_thread, argv_ptr);
    char** env = filc_check_and_get_null_terminated_string_array(my_thread, env_ptr);
    struct mac* mac = from_user_mac(my_thread, mac_ptr, filc_read_access);
    return FILC_SYSCALL(my_thread, mac_execve(fname, argv, env, mac));
}

int filc_native_zsys_eaccess(filc_thread* my_thread, filc_ptr path_ptr, int mode)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, eaccess(path, mode));
}

typedef struct {
    char* policyname;
    int call;
    int result;
} mac_syscall_data;

static void mac_syscall_callback(void* guarded_arg,
                                 void* user_arg)
{
    mac_syscall_data* data = (mac_syscall_data*)user_arg;
    data->result = mac_syscall(data->policyname, data->call, guarded_arg);
}

int filc_native_zsys_mac_syscall(filc_thread* my_thread, filc_ptr policyname_ptr, int call,
                                 filc_ptr arg_ptr)
{
    mac_syscall_data data;
    data.policyname = filc_check_and_get_tmp_str(my_thread, policyname_ptr);
    data.call = call;
    filc_call_syscall_with_guarded_ptr(my_thread, arg_ptr, mac_syscall_callback, &data);
    return data.result;
}

struct user_sf_hdtr {
	filc_ptr headers;	/* pointer to an array of header struct iovec's */
	int hdr_cnt;		/* number of header iovec's */
	filc_ptr trailers;	/* pointer to an array of trailer struct iovec's */
	int trl_cnt;		/* number of trailer iovec's */
};

static void check_user_sf_hdtr(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_PTR_FIELD(ptr, struct user_sf_hdtr, headers, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_sf_hdtr, hdr_cnt, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct user_sf_hdtr, trailers, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct user_sf_hdtr, trl_cnt, access_kind);
}

int filc_native_zsys_sendfile(filc_thread* my_thread, int fd, int s, long offset, size_t nbytes,
                              filc_ptr hdtr_ptr, filc_ptr sbytes_ptr, int flags)
{
    struct sf_hdtr* hdtr = NULL;
    if (filc_ptr_ptr(hdtr_ptr)) {
        hdtr = alloca(sizeof(struct sf_hdtr));
        check_user_sf_hdtr(hdtr_ptr, filc_read_access);
        struct user_sf_hdtr* user_hdtr = (struct user_sf_hdtr*)filc_ptr_ptr(hdtr_ptr);
        filc_ptr headers_ptr = filc_ptr_load(my_thread, &user_hdtr->headers);
        hdtr->hdr_cnt = user_hdtr->hdr_cnt;
        hdtr->headers = filc_prepare_iovec(my_thread, headers_ptr, hdtr->hdr_cnt, filc_read_access);
        filc_ptr trailers_ptr = filc_ptr_load(my_thread, &user_hdtr->trailers);
        hdtr->trl_cnt = user_hdtr->trl_cnt;
        hdtr->trailers = filc_prepare_iovec(my_thread, trailers_ptr, hdtr->trl_cnt, filc_read_access);
    }
    if (filc_ptr_ptr(sbytes_ptr))
        filc_cpt_write_int(my_thread, sbytes_ptr, sizeof(long));
    return FILC_SYSCALL(my_thread, sendfile(fd, s, offset, nbytes, hdtr,
                                            (off_t*)filc_ptr_ptr(sbytes_ptr), flags));
}

int filc_native_zsys_uuidgen(filc_thread* my_thread, filc_ptr store_ptr, int count)
{
    filc_cpt_write_int(my_thread, store_ptr, filc_mul_size(sizeof(struct uuid), count));
    return FILC_SYSCALL(my_thread, uuidgen((struct uuid*)filc_ptr_ptr(store_ptr), count));
}

int filc_native_zsys_kenv(filc_thread* my_thread, int action, filc_ptr name_ptr, filc_ptr value_ptr,
                          int len)
{
    char* name = filc_check_and_get_tmp_str(my_thread, name_ptr);
    char* value;
    switch (action) {
    case KENV_GET:
    case KENV_DUMP:
    case KENV_DUMP_LOADER:
    case KENV_DUMP_STATIC:
        filc_cpt_write_int(my_thread, value_ptr, len);
        value = (char*)filc_ptr_ptr(value_ptr);
        break;
    case KENV_SET:
        filc_cpt_read_int(my_thread, value_ptr, len);
        value = (char*)filc_ptr_ptr(value_ptr);
        break;
    case KENV_UNSET:
        value = NULL;
        len = 0;
        break;
    default:
        filc_set_errno(EINVAL);
        return -1;
    }
    return FILC_SYSCALL(my_thread, kenv(action, name, value, len));
}

int filc_native_zsys___setugid(filc_thread* my_thread, int flag)
{
    return FILC_SYSCALL(my_thread, __setugid(flag));
}

int filc_native_zsys_ksem_close(filc_thread* my_thread, long id)
{
    return FILC_SYSCALL(my_thread, ksem_close(id));
}

int filc_native_zsys_ksem_post(filc_thread* my_thread, long id)
{
    return FILC_SYSCALL(my_thread, ksem_post(id));
}

int filc_native_zsys_ksem_wait(filc_thread* my_thread, long id)
{
    return FILC_SYSCALL(my_thread, ksem_wait(id));
}

int filc_native_zsys_ksem_trywait(filc_thread* my_thread, long id)
{
    return FILC_SYSCALL(my_thread, ksem_trywait(id));
}

int filc_native_zsys_ksem_timedwait(filc_thread* my_thread, long id, filc_ptr abstime_ptr)
{
    filc_cpt_read_int(my_thread, abstime_ptr, sizeof(struct timespec));
    return FILC_SYSCALL(
        my_thread, ksem_timedwait(id, (const struct timespec*)filc_ptr_ptr(abstime_ptr)));
}

int filc_native_zsys_ksem_init(filc_thread* my_thread, filc_ptr id_ptr, unsigned value)
{
    filc_cpt_write_int(my_thread, id_ptr, sizeof(semid_t));
    return FILC_SYSCALL(my_thread, ksem_init((semid_t*)filc_ptr_ptr(id_ptr), value));
}

int filc_native_zsys_ksem_open(filc_thread* my_thread, filc_ptr id_ptr, filc_ptr name_ptr, int oflag,
                               unsigned short mode, unsigned value)
{
    char* name = filc_check_and_get_tmp_str(my_thread, name_ptr);
    filc_cpt_write_int(my_thread, id_ptr, sizeof(semid_t));
    return FILC_SYSCALL(
        my_thread, ksem_open((semid_t*)filc_ptr_ptr(id_ptr), name, oflag, mode, value));
}

int filc_native_zsys_ksem_unlink(filc_thread* my_thread, filc_ptr name_ptr)
{
    char* name = filc_check_and_get_tmp_str(my_thread, name_ptr);
    return FILC_SYSCALL(my_thread, ksem_unlink(name));
}

int filc_native_zsys_ksem_getvalue(filc_thread* my_thread, long id, filc_ptr val_ptr)
{
    filc_cpt_write_int(my_thread, val_ptr, sizeof(int));
    return FILC_SYSCALL(my_thread, ksem_getvalue(id, (int*)filc_ptr_ptr(val_ptr)));
}

int filc_native_zsys_ksem_destroy(filc_thread* my_thread, long id)
{
    return FILC_SYSCALL(my_thread, ksem_destroy(id));
}

int filc_native_zsys_audit(filc_thread* my_thread, filc_ptr record_ptr, int length)
{
    filc_cpt_read_int(my_thread, record_ptr, length);
    return FILC_SYSCALL(my_thread, audit(filc_ptr_ptr(record_ptr), length));
}

int filc_native_zsys_auditon(filc_thread* my_thread, int cmd, filc_ptr data_ptr, int length)
{
    filc_cpt_write_int(my_thread, data_ptr, length);
    return FILC_SYSCALL(my_thread, auditon(cmd, filc_ptr_ptr(data_ptr), length));
}

int filc_native_zsys_auditctl(filc_thread* my_thread, filc_ptr path_ptr)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    return FILC_SYSCALL(my_thread, auditctl(path));
}

int filc_native_zsys_getauid(filc_thread* my_thread, filc_ptr uid_ptr)
{
    filc_cpt_write_int(my_thread, uid_ptr, sizeof(au_id_t));
    return FILC_SYSCALL(my_thread, getauid((au_id_t*)filc_ptr_ptr(uid_ptr)));
}

int filc_native_zsys_setauid(filc_thread* my_thread, filc_ptr uid_ptr)
{
    filc_cpt_read_int(my_thread, uid_ptr, sizeof(au_id_t));
    return FILC_SYSCALL(my_thread, setauid((const au_id_t*)filc_ptr_ptr(uid_ptr)));
}

int filc_native_zsys_getaudit(filc_thread* my_thread, filc_ptr info_ptr)
{
    filc_cpt_write_int(my_thread, info_ptr, sizeof(struct auditinfo));
    return FILC_SYSCALL(my_thread, getaudit((struct auditinfo*)filc_ptr_ptr(info_ptr)));
}

int filc_native_zsys_setaudit(filc_thread* my_thread, filc_ptr info_ptr)
{
    filc_cpt_read_int(my_thread, info_ptr, sizeof(struct auditinfo));
    return FILC_SYSCALL(my_thread, setaudit((const struct auditinfo*)filc_ptr_ptr(info_ptr)));
}

int filc_native_zsys_getaudit_addr(filc_thread* my_thread, filc_ptr addr_ptr, int length)
{
    filc_cpt_write_int(my_thread, addr_ptr, length);
    return FILC_SYSCALL(my_thread, getaudit_addr((struct auditinfo_addr*)filc_ptr_ptr(addr_ptr),
                                                 length));
}

int filc_native_zsys_setaudit_addr(filc_thread* my_thread, filc_ptr addr_ptr, int length)
{
    filc_cpt_read_int(my_thread, addr_ptr, length);
    return FILC_SYSCALL(my_thread, setaudit_addr((const struct auditinfo_addr*)filc_ptr_ptr(addr_ptr),
                                                 length));
}

int kmq_notify(int, const struct sigevent *);
int kmq_open(const char *, int, mode_t, const struct mq_attr *);
int kmq_setattr(int, const struct mq_attr *, struct mq_attr *);
ssize_t	kmq_timedreceive(int, char *, size_t, unsigned *, const struct timespec *);
int kmq_timedsend(int, const char *, size_t, unsigned, const struct timespec *);
int kmq_unlink(const char *);

int filc_native_zsys_kmq_notify(filc_thread* my_thread, int oshandle, filc_ptr event_ptr)
{
    if (filc_ptr_ptr(event_ptr))
        filc_cpt_read_int(my_thread, event_ptr, sizeof(struct sigevent));
    return FILC_SYSCALL(my_thread, kmq_notify(oshandle,
                                              (const struct sigevent*)filc_ptr_ptr(event_ptr)));
}

int filc_native_zsys_kmq_open(filc_thread* my_thread, filc_ptr name_ptr, int oflag,
                              unsigned short mode, filc_ptr attr_ptr)
{
    char* name = filc_check_and_get_tmp_str(my_thread, name_ptr);
    if (filc_ptr_ptr(attr_ptr))
        filc_cpt_read_int(my_thread, attr_ptr, sizeof(struct mq_attr));
    return FILC_SYSCALL(my_thread, kmq_open(name, oflag, mode,
                                            (const struct mq_attr*)filc_ptr_ptr(attr_ptr)));
}

int filc_native_zsys_kmq_setattr(filc_thread* my_thread, int oshandle, filc_ptr new_attr_ptr,
                                 filc_ptr old_attr_ptr)
{
    if (filc_ptr_ptr(old_attr_ptr))
        filc_cpt_write_int(my_thread, old_attr_ptr, sizeof(struct mq_attr));
    if (filc_ptr_ptr(new_attr_ptr))
        filc_cpt_read_int(my_thread, new_attr_ptr, sizeof(struct mq_attr));
    return FILC_SYSCALL(my_thread, kmq_setattr(oshandle,
                                               (const struct mq_attr*)filc_ptr_ptr(new_attr_ptr),
                                               (struct mq_attr*)filc_ptr_ptr(old_attr_ptr)));
}

long filc_native_zsys_kmq_timedreceive(filc_thread* my_thread, int oshandle, filc_ptr buf_ptr,
                                       size_t len, filc_ptr prio_ptr, filc_ptr timeout_ptr)
{
    filc_cpt_write_int(my_thread, buf_ptr, len);
    if (filc_ptr_ptr(prio_ptr))
        filc_cpt_write_int(my_thread, prio_ptr, sizeof(unsigned));
    if (filc_ptr_ptr(timeout_ptr))
        filc_cpt_read_int(my_thread, timeout_ptr, sizeof(struct timespec));
    return FILC_SYSCALL(
        my_thread, kmq_timedreceive(oshandle, (char*)filc_ptr_ptr(buf_ptr), len,
                                    (unsigned*)filc_ptr_ptr(prio_ptr),
                                    (const struct timespec*)filc_ptr_ptr(timeout_ptr)));
}

int filc_native_zsys_kmq_timedsend(filc_thread* my_thread, int oshandle, filc_ptr buf_ptr, size_t len,
                                   unsigned prio, filc_ptr timeout_ptr)
{
    filc_cpt_read_int(my_thread, buf_ptr, len);
    if (filc_ptr_ptr(timeout_ptr))
        filc_cpt_read_int(my_thread, timeout_ptr, sizeof(struct timespec));
    return FILC_SYSCALL(my_thread, kmq_timedsend(oshandle, (const char*)filc_ptr_ptr(buf_ptr), len,
                                                 prio,
                                                 (const struct timespec*)filc_ptr_ptr(timeout_ptr)));
}

int filc_native_zsys_kmq_unlink(filc_thread* my_thread, filc_ptr name_ptr)
{
    char* name = filc_check_and_get_tmp_str(my_thread, name_ptr);
    return FILC_SYSCALL(my_thread, kmq_unlink(name));
}

static void setup_umtx_timeout(filc_thread* my_thread, void** uaddr, void** uaddr2,
                               filc_ptr uaddr_ptr, filc_ptr uaddr2_ptr)
{
    if (filc_ptr_ptr(uaddr2_ptr)) {
        if ((size_t)filc_ptr_ptr(uaddr_ptr) > sizeof(struct timespec)) {
            *uaddr = filc_ptr_ptr(uaddr_ptr);
            filc_cpt_read_int(my_thread, uaddr2_ptr, (size_t)*uaddr);
        } else
            filc_cpt_read_int(my_thread, uaddr2_ptr, sizeof(struct timespec));
        *uaddr2 = filc_ptr_ptr(uaddr2_ptr);
    }
}

static void** from_raw_ptr_array(filc_thread* my_thread, filc_ptr ptrs_array, int nptrs)
{
    void** result = filc_bmalloc_allocate_tmp(my_thread, filc_mul_size(sizeof(void*), nptrs));
    size_t index;
    for (index = (size_t)nptrs; index--;) {
        filc_ptr ptr_ptr = filc_ptr_with_offset(ptrs_array, filc_mul_size(sizeof(filc_ptr), index));
        filc_check_read_ptr(ptr_ptr, NULL);
        result[index] = filc_ptr_ptr(
            filc_ptr_load_with_manual_tracking((filc_ptr*)filc_ptr_ptr(ptr_ptr)));
    }
    return result;
}

int filc_native_zsys__umtx_op(filc_thread* my_thread, filc_ptr obj_ptr, int op, unsigned long val,
                              filc_ptr uaddr_ptr, filc_ptr uaddr2_ptr)
{
    void* obj = filc_ptr_ptr(obj_ptr);
    void* uaddr = NULL;
    void* uaddr2 = NULL;
    switch (op) {
    case UMTX_OP_MUTEX_LOCK:
    case UMTX_OP_MUTEX_WAIT:
        filc_cpt_write_int(my_thread, obj_ptr, sizeof(struct umutex));
        setup_umtx_timeout(my_thread, &uaddr, &uaddr2, uaddr_ptr, uaddr2_ptr);
        break;
    case UMTX_OP_MUTEX_UNLOCK:
    case UMTX_OP_MUTEX_TRYLOCK:
    case UMTX_OP_MUTEX_WAKE2:
        filc_cpt_write_int(my_thread, obj_ptr, sizeof(struct umutex));
        break;
    case UMTX_OP_SET_CEILING:
        filc_cpt_write_int(my_thread, obj_ptr, sizeof(struct umutex));
        filc_cpt_write_int(my_thread, uaddr_ptr, sizeof(unsigned));
        uaddr = filc_ptr_ptr(uaddr_ptr);
        break;
    case UMTX_OP_WAIT:
        filc_cpt_read_int(my_thread, obj_ptr, sizeof(long));
        setup_umtx_timeout(my_thread, &uaddr, &uaddr2, uaddr_ptr, uaddr2_ptr);
        break;
    case UMTX_OP_WAIT_UINT:
    case UMTX_OP_WAIT_UINT_PRIVATE:
        filc_cpt_read_int(my_thread, obj_ptr, sizeof(unsigned));
        setup_umtx_timeout(my_thread, &uaddr, &uaddr2, uaddr_ptr, uaddr2_ptr);
        break;
    case UMTX_OP_WAKE:
    case UMTX_OP_WAKE_PRIVATE:
        /* We can pass the ptr through to the kernel without checks since it's just being used as
           the queue key. */
        break;
    case UMTX_OP_CV_WAIT:
        filc_cpt_write_int(my_thread, obj_ptr, sizeof(struct ucond));
        filc_cpt_write_int(my_thread, uaddr_ptr, sizeof(struct umutex));
        uaddr = filc_ptr_ptr(uaddr_ptr);
        filc_cpt_read_int(my_thread, uaddr2_ptr, sizeof(struct timespec));
        uaddr2 = filc_ptr_ptr(uaddr2_ptr);
        break;
    case UMTX_OP_CV_SIGNAL:
    case UMTX_OP_CV_BROADCAST:
        filc_cpt_write_int(my_thread, obj_ptr, sizeof(struct ucond));
        break;
    case UMTX_OP_RW_RDLOCK:
    case UMTX_OP_RW_WRLOCK:
        filc_cpt_write_int(my_thread, obj_ptr, sizeof(struct urwlock));
        setup_umtx_timeout(my_thread, &uaddr, &uaddr2, uaddr_ptr, uaddr2_ptr);
        break;
    case UMTX_OP_RW_UNLOCK:
        filc_cpt_write_int(my_thread, obj_ptr, sizeof(struct urwlock));
        break;
    case UMTX_OP_SEM2_WAKE:
        filc_cpt_write_int(my_thread, obj_ptr, sizeof(struct _usem2));
        break;
    case UMTX_OP_SEM2_WAIT:
        filc_cpt_write_int(my_thread, obj_ptr, sizeof(struct _usem2));
        setup_umtx_timeout(my_thread, &uaddr, &uaddr2, uaddr_ptr, uaddr2_ptr);
        break;
    case UMTX_OP_SET_MIN_TIMEOUT:
        obj = NULL;
        break;
    case UMTX_OP_NWAKE_PRIVATE: {
        obj = from_raw_ptr_array(my_thread, obj_ptr, val);
        break;
    }
    case UMTX_OP_SHM:
        /* Not 100% sure about this, but whatever.

           I think this is OK because the uaddr_ptr is just used as a key and the kernel doesn't
           modify anything at this address. */
        obj = NULL;
        uaddr = filc_ptr_ptr(uaddr_ptr);
        break;
    case UMTX_OP_ROBUST_LISTS:
        /* We cannot support this feature, since it involves the kernel fiddling with pointers in
           our address space. It's just too crazy of a feature for comfort. Also, nobody uses robust
           mutexes. They are the ultimate checkbox feature. */
        filc_set_errno(ENOSYS);
        return -1;
    default:
        filc_set_errno(EINVAL);
        return -1;
    }
    return FILC_SYSCALL(my_thread, _umtx_op(obj, op, val, uaddr, uaddr2));
}

void filc_native_zsys_abort2(filc_thread* my_thread, filc_ptr why_ptr, int nargs, filc_ptr args_ptr)
{
    char* why = filc_check_and_get_tmp_str(my_thread, why_ptr);
    void** args = from_raw_ptr_array(my_thread, args_ptr, nargs);
    filc_exit(my_thread);
    abort2(why, nargs, args);
    filc_safety_panic(NULL, "abort2 failed.");
}

int filc_native_zsys_fexecve(filc_thread* my_thread, int fd, filc_ptr argv_ptr, filc_ptr envp_ptr)
{
    char** argv = filc_check_and_get_null_terminated_string_array(my_thread, argv_ptr);
    char** envp = filc_check_and_get_null_terminated_string_array(my_thread, envp_ptr);
    return FILC_SYSCALL(my_thread, fexecve(fd, argv, envp));
}

int filc_native_zsys_cpuset_getaffinity(filc_thread* my_thread, int level, int which, long long id,
                                        size_t setsize, filc_ptr mask_ptr)
{
    filc_cpt_write_int(my_thread, mask_ptr, setsize);
    return FILC_SYSCALL(my_thread, cpuset_getaffinity(level, which, id, setsize,
                                                      (cpuset_t*)filc_ptr_ptr(mask_ptr)));
}

int filc_native_zsys_cpuset_setaffinity(filc_thread* my_thread, int level, int which, long long id,
                                        size_t setsize, filc_ptr mask_ptr)
{
    filc_cpt_read_int(my_thread, mask_ptr, setsize);
    return FILC_SYSCALL(my_thread, cpuset_setaffinity(level, which, id, setsize,
                                                      (const cpuset_t*)filc_ptr_ptr(mask_ptr)));
}

int filc_native_zsys_cpuset(filc_thread* my_thread, filc_ptr setid_ptr)
{
    filc_cpt_write_int(my_thread, setid_ptr, sizeof(cpusetid_t));
    return FILC_SYSCALL(my_thread, cpuset((cpusetid_t*)filc_ptr_ptr(setid_ptr)));
}

int filc_native_zsys_cpuset_setid(filc_thread* my_thread, int which, long long id, int setid)
{
    return FILC_SYSCALL(my_thread, cpuset_setid(which, id, setid));
}

int filc_native_zsys_cpuset_getid(filc_thread* my_thread, int level, int which, long long id,
                                  filc_ptr setid_ptr)
{
    filc_cpt_write_int(my_thread, setid_ptr, sizeof(cpusetid_t));
    return FILC_SYSCALL(my_thread, cpuset_getid(level, which, id,
                                                (cpusetid_t*)filc_ptr_ptr(setid_ptr)));
}

#endif /* PAS_ENABLE_FILC && FILC_FILBSD */

#endif /* LIBPAS_ENABLED */
