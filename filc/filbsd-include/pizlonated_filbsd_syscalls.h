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
int zsys_lchflags(const char* path, unsigned long flags);
int zsys_fchflags(int fd, unsigned long flags);
int zsys_chflagsat(int fd, const char* path, unsigned long flags, int atflag);
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
int zsys_ffclock_getcounter(unsigned long long* ffcount);
int zsys_ffclock_getestimate(void* cest);
int zsys_ffclock_setestimate(void* cest);
int zsys_clock_getcpuclockid2(long long id, int which, void* clock_id);
int zsys_minherit(void* addr, __SIZE_TYPE__ len, int inherit);
int zsys_issetugid(void);
int zsys_aio_read(void* iocb);
int zsys_aio_readv(void* iocb);
int zsys_aio_write(void* iocb);
int zsys_aio_writev(void* iocb);
int zsys_lio_listio(int mode, void** list, int nent, void* sig);
int zsys_aio_return(void* iocb);
int zsys_aio_suspend(const void* const* iocbs, int niocb, const void* timeout);
int zsys_aio_error(const void* iocb);
int zsys_aio_cancel(int fildes, void* iocb);
long zsys_aio_waitcomplete(void** iocbp, void* timeout);
int zsys_aio_fsync(int op, void* iocb);
int zsys_aio_mlock(void* iocb);
int zsys_fhopen(const void* fhp, int flags);
int zsys_fhstat(const void* fhp, void* sb);
int zsys_fhstatfs(const void* fhp, void* buf);
int zsys_modnext(int modid);
int zsys_modfnext(int modid);
int zsys_modstat(int modid, void* stat);
int zsys_modfind(const char* modname);
int zsys_kldload(const char* file);
int zsys_kldunload(int fileid);
int zsys_kldunloadf(int fileid, int flags);
int zsys_kldfind(const char* file);
int zsys_kldnext(int fileid);
int zsys_getresgid(unsigned* rgid, unsigned* egid, unsigned* sgid);
int zsys_getresuid(unsigned* ruid, unsigned* euid, unsigned* suid);
int zsys_setresgid(unsigned rgid, unsigned egid, unsigned sgid);
int zsys_setresuid(unsigned ruid, unsigned euid, unsigned suid);
int zsys_kldfirstmod(int fileid);
int zsys_kldstat(int fileid, void* stat);
int zsys___getcwd(char* buf, __SIZE_TYPE__ size);
int zsys_sched_setparam(int pid, const void* param);
int zsys_sched_getparam(int pid, void* param);
int zsys_sched_setscheduler(int pid, int policy, const void* param);
int zsys_sched_getscheduler(int pid);
int zsys_sched_get_priority_max(int policy);
int zsys_sched_get_priority_min(int policy);
int zsys_sched_rr_get_interval(int pid, void* interval);
int zsys_utrace(const void* addr, __SIZE_TYPE__ len);
int zsys_kldsym(int fileid, int cmd, void* data);
int zsys_jail(void* jail);
int zsys_jail_attach(int jid);
int zsys_jail_remove(int jid);
int zsys_jail_get(void* iov, unsigned niov, int flags);
int zsys_jail_set(void* iov, unsigned niov, int flags);
int zsys___acl_aclcheck_fd(int fd, int type, void* aclp);
int zsys___acl_aclcheck_file(const char* path, int type, void* aclp);
int zsys___acl_aclcheck_link(const char* path, int type, void* aclp);
int zsys___acl_delete_fd(int fd, int type);
int zsys___acl_delete_file(const char* path, int type);
int zsys___acl_delete_link(const char* path, int type);
int zsys___acl_get_fd(int fd, int type, void* aclp);
int zsys___acl_get_file(const char* path, int type, void* aclp);
int zsys___acl_get_link(const char* path, int type, void* aclp);
int zsys___acl_set_fd(int fd, int type, void* aclp);
int zsys___acl_set_file(const char* path, int type, void* aclp);
int zsys___acl_set_link(const char* path, int type, void* aclp);
int zsys_extattr_delete_fd(int fd, int attrnamespace, const char* attrname);
int zsys_extattr_delete_file(const char* path, int attrnamespace, const char* attrname);
int zsys_extattr_delete_link(const char* path, int attrnamespace, const char* attrname);
long zsys_extattr_get_fd(int fd, int attrnamespace, const char* attrname, void* data,
                         __SIZE_TYPE__ nbytes);
long zsys_extattr_get_file(const char* path, int attrnamespace, const char* attrname, void* data,
                           __SIZE_TYPE__ nbytes);
long zsys_extattr_get_link(const char* path, int attrnamespace, const char* attrname, void* data,
                           __SIZE_TYPE__ nbytes);
long zsys_extattr_list_fd(int fd, int attrnamespace, void* data, __SIZE_TYPE__ nbytes);
long zsys_extattr_list_file(const char* path, int attrnamespace, void* data, __SIZE_TYPE__ nbytes);
long zsys_extattr_list_link(const char* path, int attrnamespace, void* data, __SIZE_TYPE__ nbytes);
long zsys_extattr_set_fd(int fd, int attrnamespace, const char* attrname, const void* data,
                         __SIZE_TYPE__ nbytes);
long zsys_extattr_set_file(const char* path, int attrnamespace, const char* attrname,
                           const void* data, __SIZE_TYPE__ nbytes);
long zsys_extattr_set_link(const char* path, int attrnamespace, const char* attrname,
                           const void* data, __SIZE_TYPE__ nbytes);
int zsys_extattrctl(const char* path, int cmd, const char* filename, int attrnamespace,
                    const char* attrname);
int zsys_kqueue(void);
int zsys_kqueuex(unsigned flags);
int zsys_kevent(int kq, const void* changelist, int nchanges, void* eventlist, int nevents,
                const void* timeout);
int zsys___mac_get_fd(int fd, void* mac);
int zsys___mac_get_file(const char* path, void* mac);
int zsys___mac_get_link(const char* path, void* mac);
int zsys___mac_get_pid(int pid, void* mac);
int zsys___mac_get_proc(void* mac);
int zsys___mac_set_fd(int fd, void* mac);
int zsys___mac_set_file(const char* path, void* mac);
int zsys___mac_set_link(const char* path, void* mac);
int zsys___mac_set_proc(void* mac);
int zsys___mac_execve(const char* fname, char** argv, char** env, void* mac);
int zsys_eaccess(const char* path, int mode);
int zsys_mac_syscall(const char* policyname, int call, void* arg);
int zsys_sendfile(int fd, int s, long offset, __SIZE_TYPE__ nbytes, void* hdtr, long* sbytes,
                  int flags);
int zsys_uuidgen(void* store, int count);
int zsys_kenv(int action, const char* name, char* value, int len);
int zsys___setugid(int flag);
int zsys_ksem_close(long id);
int zsys_ksem_post(long id);
int zsys_ksem_wait(long id);
int zsys_ksem_trywait(long id);
int zsys_ksem_timedwait(long id, const void* abstime);
int zsys_ksem_init(long* idp, unsigned value);
int zsys_ksem_open(long* idp, const char* name, int oflag, unsigned short mode, unsigned value);
int zsys_ksem_unlink(const char* name);
int zsys_ksem_getvalue(long id, int* val);
int zsys_ksem_destroy(long id);
int zsys_audit(const void* record, int length);
int zsys_auditon(int cmd, void* data, int length);
int zsys_auditctl(const char* path);
int zsys_getauid(unsigned* uid);
int zsys_setauid(const unsigned* uid);
int zsys_getaudit(void* info);
int zsys_setaudit(const void* info);
int zsys_getaudit_addr(void* addr, int length);
int zsys_setaudit_addr(const void* addr, int length);
int zsys_kmq_notify(int oshandle, const void* event);
int zsys_kmq_open(const char* name, int oflag, unsigned short mode, const void* attr);
int zsys_kmq_setattr(int oshandle, const void* new_attr, void* old_attr);
long zsys_kmq_timedreceive(int oshandle, char* buf, __SIZE_TYPE__ len, unsigned* prio,
                           const void* timeout);
int zsys_kmq_timedsend(int oshandle, const char* buf, __SIZE_TYPE__ len, unsigned prio,
                       const void* timeout);
int zsys_kmq_unlink(const char* name);
int zsys__umtx_op(void* obj, int op, unsigned long value, void* uaddr, void* uaddr2);
void zsys_abort2(const char* why, int nargs, void** args);
int zsys_futimesat(int fd, const char* path, const void* times);
int zsys_fexecve(int fd, char*const* argv, char*const* envp);
int zsys_cpuset_getaffinity(int level, int which, long long id, __SIZE_TYPE__ setsize, void* mask);
int zsys_cpuset_setaffinity(int level, int which, long long id, __SIZE_TYPE__ setsize,
                            const void* mask);
int zsys_cpuset(int* setid);
int zsys_cpuset_setid(int which, long long id, int setid);
int zsys_cpuset_getid(int level, int which, long long id, int* setid);

#endif /* PIZLONATED_FILBSD_SYSCALLS_H */

