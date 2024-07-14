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

#ifndef PIZLONATED_MUSL_SYSCALLS_H
#define PIZLONATED_MUSL_SYSCALLS_H

#include <pizlonated_common_syscalls.h>

/* This file defines pizlonated syscalls consumed by musl.

   Pizlonated musl doesn't know what OS it's running on. It just makes these syscalls. Then,
   filc_runtime.c figures out how to "port" them to whatever OS you're running on. So, many of
   these functions aren't real syscalls.

   There's no guarantee that the APIs in this file will be stable over time. */

void* zsys_getpwuid(unsigned uid);
int zsys_isatty(int fd);
int zsys_getaddrinfo(const char* node, const char* service, const void* hints, void** res);
int zsys_uname(void* buf);
void* zsys_getpwnam(const char* name);
int zsys_setgroups(__SIZE_TYPE__ size, const unsigned* list);
void* zsys_opendir(const char* name);
void* zsys_fdopendir(int fd);
int zsys_closedir(void* dirp);
void* zsys_readdir(void* dirp);
void zsys_rewinddir(void* dirp);
void zsys_seekdir(void* dirp, long loc);
long zsys_telldir(void* dirp);
int zsys_dirfd(void* dirp);
void zsys_closelog(void);
void zsys_openlog(const char* ident, int option, int facility);
int zsys_setlogmask(int mask);
void zsys_syslog(int priority, const char* msg); /* formatting is up to libc and thankfully musl's printf
                                                    always just supports %m! */
int zsys_getgrouplist(const char* user, unsigned group, unsigned* groups, int* ngroups);
int zsys_initgroups(const char* user, unsigned gid);
int zsys_openpty(int* masterfd, int* slavefd, char* name, const void* term, const void* win);
int zsys_ttyname_r(int fd, char* buf, __SIZE_TYPE__ buflen);
void* zsys_getgrnam(const char* name);
void zsys_endutxent(void);
void* zsys_getutxent(void);
void* zsys_getutxid(const void* utmpx);
void* zsys_getutxline(const void* utmpx);
void* zsys_pututxline(const void* utmpx);
void zsys_setutxent(void);
void* zsys_getlastlogx(unsigned uid, void* lastlogx); /* Only available on Darwin. */
void* zsys_getlastlogxbyname(const char* name, void* lastlogx); /* Only available on Darwin. */
long zsys_sysconf_override(int name); /* If libpizlo wants to override this value, it returns
                                         it; otherwise it returns -1 with ENOSYS. Many sysconfs
                                         are handled by musl! */
int zsys_numcores(void);
int zsys_getentropy(void* buffer, __SIZE_TYPE__ len);
int zsys_posix_spawn(int* pid, const char* path, const void* file_actions, const void* attrp,
                     char*const* argv, char*const* envp);
int zsys_posix_spawnp(int* pid, const char* path, const void* file_actions, const void* attrp,
                      char*const* argv, char*const* envp);
int zsys_sendfile(int out_fd, int in_fd, long* offset, __SIZE_TYPE__ count);

#endif /* PIZLONATED_MUSL_SYSCALLS_H */

