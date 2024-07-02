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

#ifndef PIZLONATED_FILBSD_SYSCALLS_H
#define PIZLONATED_FILBSD_SYSCALLS_H

#include <pizlonated_common_syscalls.h>

int zsys_unmount(const char* dir, int flags);
int zsys_nmount(void* iov, unsigned niov, int flags);
int zsys_chflags(const char* path, unsigned long flags);
int zsys_fchflags(int fd, unsigned long flags);
int zsys_profil(char* samples, __SIZE_TYPE__ size, unsigned long offset, int scale);
int zsys_setlogin(const char* name);
int zsys_acct(const char* file);
int zsys_reboot(int howto);
int zsys_revoke(const char* path);
int zsys_ktrace(const char *tracefile, int ops, int trpoints, int pid);
int zsys_setgroups(int size, const unsigned* list);
int zsys_madvise(void* addr, __SIZE_TYPE__ len, int behav);
int zsys_mincore(void* addr, __SIZE_TYPE__ len, char* vec);
int zsys_swapon(const char* special);
int zsys_swapoff(const char* special, unsigned flags);
int zsys_getdtablesize(void);
int zsys_getpriority(int which, int who);
int zsys_setpriority(int which, int who, int prio);
int zsys_gettimeofday(void* tp, void* tzp);
int zsys_settimeofday(const void* tp, const void* tzp);
int zsys_getrusage(int who, void* rusage);
int zsys_flock(int fd, int operation);
int zsys_mkfifo(const char* path, unsigned short mode);
int zsys_chmod(const char* pathname, unsigned short mode);
int zsys_fchmod(int fd, unsigned short mode);
int zsys_mkdirat(int dirfd, const char* pathname, unsigned short mode);
int zsys_mkdir(const char* path, unsigned short mode);
int zsys_utimes(const char* path, const void* times);
int zsys_lutimes(const char* path, const void* times);
int zsys_adjtime(const void* delta, void* olddelta);
int zsys_quotactl(const char* path, int cmd, int id, void* addr);
int zsys_nlm_syscall(int debug_level, int grace_period, int addr_count, char** addrs);
int zsys_nfssvc(int flags, void* argstructp);
int zsys_rtprio(int function, int pid, void* rtp);
int zsys_rtprio_thread(int function, int lwpid, void* rtp);
int zsys_getfh(const char* path, void* fhp);
int zsys_lgetfh(const char* path, void* fhp);
int zsys_getfhat(int fd, const char* path, void* fhp, int flag);
int zsys_setfib(int fib);
int zsys_ntp_adjtime(void* timex);
int zsys_ntp_gettime(void* timeval);
long zsys_pathconf(const char* path, int name);
long zsys_lpathconf(const char* path, int name);
long zsys_fpathconf(int fd, int name);
int zsys_mlock(const void* addr, __SIZE_TYPE__ len);
int zsys_munlock(const void* addr, __SIZE_TYPE__ len);
int zsys_setrlimit(int resource, const void* rlp);
int zsys___sysctl(const int* name, unsigned namelen, void* oldp, __SIZE_TYPE__ oldlenp,
                  const void* newp, __SIZE_TYPE__ newlen);
int zsys_undelete(const char* path);
int zsys_semget(long key, int nsems, int flag);
int zsys_semctl(int semid, int semnum, int cmd, ...);
int zsys_semop(int semid, void* array, __SIZE_TYPE__ nops);
int zsys_shmget(long key, __SIZE_TYPE__ size, int flag);
int zsys_shmctl(int shmid, int cmd, void* buf);
void* zsys_shmat(int shmid, const void* addr, int flag);
int zsys_shmdt(const void* addr);
int zsys_msgget(long key, int msgflg);
int zsys_msgctl(int msgid, int cmd, void* buf);
long zsys_msgrcv(int msgid, void* msgp, __SIZE_TYPE__ msgsz, long msgtyp, int msgflg);
int zsys_msgsnd(int msgid, const void* msgp, __SIZE_TYPE__ msgsz, int msgflg);
int zsys_futimes(int fd, const void* times);
int zsys_ktimer_create(int clock, void* sigev, int* oshandle);
int zsys_clock_settime(int clock_id, const void* tp);
int zsys_clock_getres(int clock_id, void* tp);
int zsys_ktimer_delete(int oshandle);
int zsys_ktimer_gettime(int oshandle, void* itimerp);
int zsys_ktimer_getoverrun(int oshandle);
int zsys_ktimer_settime(int oshandle, int flags, const void* new_value, void* old_value);

#endif /* PIZLONATED_FILBSD_SYSCALLS_H */

