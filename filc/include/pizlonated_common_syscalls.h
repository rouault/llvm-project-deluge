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

#ifndef PIZLONATED_COMMON_SYSCALLS_H
#define PIZLONATED_COMMON_SYSCALLS_H

#include <stdfil.h>

/* This file defines pizlonated syscall APIs that are common between all POSIX target variants.

   It's possible that libpizlo does something a bit different depending on the target (musl
   or filbsdrt), but the signature is the same regardless.

   There's no guarantee that the APIs in this file will be stable over time. */

int zsys_ioctl(int fd, unsigned long request, ...);
long zsys_writev(int fd, const void* iov, int iovcnt);
long zsys_read(int fd, void* buf, __SIZE_TYPE__ size);
long zsys_readv(int fd, const void* iov, int iovcnt);
long zsys_write(int fd, const void* buf, __SIZE_TYPE__ size);
int zsys_close(int fd);
long zsys_lseek(int fd, long offset, int whence);
void zsys_exit(int return_code);
unsigned zsys_getuid(void);
unsigned zsys_geteuid(void);
unsigned zsys_getgid(void);
unsigned zsys_getegid(void);
int zsys_open(const char* path, int flags, ...);
int zsys_getpid(void);
int zsys_clock_gettime(int clock_id, unsigned long long* timespec_ptr);
int zsys_fstatat(int fd, const char* path, void* buf, int flag);
int zsys_fstat(int fd, void* buf);
int zsys_fcntl(int fd, int cmd, ...);
int zsys_sigaction(int signum, const void* act, void* oact);
int zsys_pipe(int fds[2]);
int zsys_select(int nfds, void* reafds, void* writefds, void* errorfds, void* timeout);
void zsys_sched_yield(void);
int zsys_socket(int domain, int type, int protocol);
int zsys_setsockopt(int sockfd, int level, int optname, const void* optval, unsigned optlen);
int zsys_bind(int sockfd, const void* addr, unsigned addrlen);
int zsys_connect(int sockfd, const void* addr, unsigned addrlen);
int zsys_getsockname(int sockfd, void* addr, unsigned* addrlen);
int zsys_getsockopt(int sockfd, int level, int optname, void* optval, unsigned* optlen);
int zsys_getpeername(int sockfd, void* addr, unsigned* addrlen);
long zsys_sendto(int sockfd, const void* buf, __SIZE_TYPE__ len, int flags,
                 const void* addr, unsigned addrlen);
long zsys_recvfrom(int sockfd, void* buf, __SIZE_TYPE__ len, int flags,
                   void* addr, unsigned* addrlen);
int zsys_getrlimit(int resource, void* rlim);
unsigned zsys_umask(unsigned mask);
int zsys_getitimer(int which, void* curr_value);
int zsys_setitimer(int which, const void* new_value, void* old_value);
int zsys_pause(void);
int zsys_pselect(int nfds, void* readfds, void* writefds, void* exceptfds, const void* timeout,
                 const void* sigmask);
int zsys_getpeereid(int fd, unsigned* uid, unsigned* gid);
int zsys_kill(int pid, int sig);
int zsys_raise(int sig);
int zsys_dup(int fd);
int zsys_dup2(int oldfd, int newfd);
int zsys_sigprocmask(int how, const void* set, void* oldset); /* This is pthread_sigmask, but sets
                                                                 errno and returns -1 on error. */
int zsys_chdir(const char* path);
int zsys_fork(void);
int zsys_waitpid(int pid, int* status, int options);
int zsys_listen(int sockfd, int backlog);
int zsys_accept(int sockfd, void* addr, unsigned* addrlen);
int zsys_socketpair(int domain, int type, int protocol, int* sv);
int zsys_setsid(void);
int zsys_execve(const char* pathname, char*const* argv, char*const* envp);
int zsys_getppid(void);
int zsys_chroot(const char* path);
int zsys_setuid(unsigned uid);
int zsys_seteuid(unsigned uid);
int zsys_setreuid(unsigned ruid, unsigned euid);
int zsys_setgid(unsigned gid);
int zsys_setegid(unsigned gid);
int zsys_setregid(unsigned rgid, unsigned egid);
int zsys_nanosleep(const void* req, void* rem);
long zsys_readlink(const char* path, char* buf, __SIZE_TYPE__ bufsize);
int zsys_chown(const char* pathname, unsigned owner, unsigned group);
long zsys_sendmsg(int sockfd, const void* msg, int flags);
long zsys_recvmsg(int sockfd, void* msg, int flags);
int zsys_rename(const char* oldname, const char* newname);
int zsys_unlink(const char* path);
int zsys_link(const char* oldname, const char* newname);
void* zsys_mmap(void* address, __SIZE_TYPE__ length, int prot, int flags, int fd, long offset);
int zsys_munmap(void* address, __SIZE_TYPE__ length);
int zsys_ftruncate(int fd, long length);
char* zsys_getcwd(char* buf, __SIZE_TYPE__ size);
void* zsys_dlopen(const char* filename, int flags); /* FIXME: we should add dlclose support eventually,
                                                       but that would require changing the ABI so that
                                                       globals are GC-allocated. Which is fine, we could
                                                       do that. */
void* zsys_dlsym(void* handle, const char* symbol);
int zsys_poll(void* pollfds, unsigned long nfds, int timeout);
int zsys_faccessat(int dirfd, const char* pathname, int mode, int flags);
int zsys_sigwait(const void* sigmask, int* sig);
int zsys_fsync(int fd);
int zsys_shutdown(int fd, int how);
int zsys_rmdir(const char* path);
int zsys_futimens(int fd, const void* times);
int zsys_fchown(int fd, unsigned uid, unsigned gid);
int zsys_fchdir(int fd);
void zsys_sync(void);
int zsys_access(const char* path, int mode);
int zsys_symlink(const char* oldname, const char* newname);
int zsys_mprotect(void* addr, __SIZE_TYPE__ len, int prot);
int zsys_getgroups(int size, unsigned* list);
int zsys_getpgrp(void);
int zsys_getpgid(int pid);
int zsys_setpgid(int pid, int pgrp);

#endif /* PIZLONATED_COMMON_SYSCALLS_H */
