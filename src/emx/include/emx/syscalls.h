/* emx/syscalls.h (emx+gcc) */

#ifndef _EMX_SYSCALLS_H
#define _EMX_SYSCALLS_H

#include <sys/types.h>
#include <sys/_sigset.h>

#if defined (__cplusplus)
extern "C" {
#endif

/* __findfirst(), __findnext() */

#define _A_NORMAL 0x00  /* No attributes */
#define _A_RDONLY 0x01  /* Read-only     */
#define _A_HIDDEN 0x02  /* Hidden        */
#define _A_SYSTEM 0x04  /* System        */
#define _A_VOLID  0x08  /* Volume label  */
#define _A_SUBDIR 0x10  /* Directory     */
#define _A_ARCH   0x20  /* Archive       */

/* __open() */

#define _SO_CREAT       0x00010000
#define _SO_EXCL        0x00020000
#define _SO_TRUNC       0x00040000
#define _SO_NOINHERIT   0x00080000
#define _SO_SYNC        0x00100000
#define _SO_SIZE        0x00200000

struct hostent;
struct netent;
struct protoent;
struct servent;
struct sigaction;
struct sockaddr;
struct stat;
struct timeb;
struct timeval;

#pragma pack(1)
struct _find
{
    /** Size of file, in number of bytes. */
    off_t           cbFile;
    /** Last written (OS/2 & DOS time). */
    unsigned short  time;
    /** Last written (OS/2 & DOS date). */
    unsigned short  date;
    /** File attributes. */
    unsigned char   attr;
    /** File name */
    char            szName[257];
    /** inode number or 0. */
    ino_t           ino;
};
#pragma pack()

struct _new_proc
{
  unsigned long  arg_off;
  unsigned long  env_off;
  unsigned long  fname_off;
  unsigned short arg_sel;
  unsigned short env_sel;
  unsigned short fname_sel;
  unsigned short arg_count;
  unsigned short arg_size;
  unsigned short env_count;
  unsigned short env_size;
  unsigned       mode;
};

struct _profil
{
  unsigned cb;
  void *buff;
  unsigned bufsiz;
  unsigned offset;
  unsigned scale;
};

struct _recvfrom
{
  int handle;
  void *buf;
  int len;
  unsigned flags;
  struct sockaddr *from;
  int *pfromlen;
};

struct _select
{
  int              nfds;
  struct fd_set * readfds;
  struct fd_set * writefds;
  struct fd_set * exceptfds;
  struct timeval * timeout;
};

struct _sendto
{
  int handle;
  const void *buf;
  int len;
  unsigned flags;
  const struct sockaddr *to;
  int tolen;
};


//dead int __accept (int handle, struct sockaddr *addr, int *paddrlen);
unsigned __alarm (unsigned sec);
//dead int __bind (int handle, const struct sockaddr *addr, int addrlen);
void *__brk (void *addr);
void __cgets (char *buffer);
//dead int __chdir (const char *name);
int __chmod (const char *name, int flag, int attr);
int __chdrive (char drive);
//dead int __chsize (int handle, off_t length);
long long __clock (void);
int __close (int handle);
//dead int __connect (int handle, const struct sockaddr *addr, int addrlen);
int __core (int handle);
int __dup (int handle);
int __dup2 (int handle1, int handle2);
int __endthread (void *pvThrd);
int __execname (char *buf, size_t bufsize);
void __exit (int ret) __attribute__ ((__noreturn__));
/*int __fcntl (int handle, int request, int arg);*/
int __filesys (const char *drive, char *name, size_t size);
int __findfirst (const char *name, int attr, struct _find *fp);
int __findnext (struct _find *fp);
int __fork (void);
//dead int __fstat (int handle, struct stat *buffer);
int __fsync (int handle);
void __ftime (struct timeb *ptr);
//dead int __ftruncate (int handle, off_t length);
//dead int __getcwd (char *buffer, char drive);
char __getdrive (void);
//dead int __gethostbyaddr (const char *addr, int len, int type,
//dead     struct hostent **dst);
//dead int __gethostbyname (const char *name, struct hostent **dst);
//dead int __gethostid (int *dst);
//dead int __gethostname (char *name, int len);
//dead int __getnetbyname (const char *name, struct netent **dst);
//dead int __getnetbyaddr (long net, struct netent **dst);
//dead int __getpeername (int handle, struct sockaddr *name, int *pnamelen);
int __getpid (void);
int __getppid (void);
//dead int __getprotobyname (const char *name, struct protoent **dst);
//dead int __getprotobynumber (int prot, struct protoent **dst);
//dead int __getservbyname (const char *name, const char *proto,
//dead     struct servent **dst);
//dead int __getservbyport (int port, const char *proto, struct servent **dst);
//dead int __getsockhandle (int handle);
//dead int __getsockname (int handle, struct sockaddr *name, int *pnamelen);
//dead int __getsockopt (int handle, int level, int optname, void *optval,
//dead     int *poptlen);
int __imphandle (int handle);
//dead int __impsockhandle (int handle, int flags);
int __ioctl1 (int handle, int code);
int __ioctl2 (int handle, unsigned long request, int arg);
int __kill (int pid, int sig);
int __listen (int handle, int backlog);
//dead off_t __lseek (int handle, off_t offset, int origin);
int __memavail (void);
//dead int __mkdir (const char *name);
int __newthread (int tid);
struct __libc_FileHandle;
//dead int __open (const char *name, int flags, off_t size, mode_t cmode, unsigned fLibc, struct __libc_FileHandle **pFH);
int __pause (void);
int __pipe(int *two_handles, int pipe_size, struct __libc_FileHandle **ppFHRead, struct __libc_FileHandle **ppFHWrite);
int __profil (const struct _profil *p);
int __ptrace (int request, int pid, int addr, int data);
int __raise (int sig);
int __read (int handle, void *buf, size_t nbyte);
int __read_kbd (int echo, int wait, int sig);
int __recv (int handle, void *buf, int len, unsigned flags);
int __recvfrom (const struct _recvfrom *args);
int __remove (const char *name);
int __rename (const char *old_name, const char *new_name);
//dead int __rmdir (const char *name);
void *__sbrk (int incr);
void __scrsize (int *dst);
int __select(int nfds, struct fd_set *readfds, struct fd_set *writefds,
             struct fd_set *exceptfds, struct timeval *tv);
//dead int __send (int handle, const void *buf, int len, unsigned flags);
//dead int __sendto (const struct _sendto *args);
//dead int __setsockopt (int handle, int level, int optname, const void *optval,
//dead     int optlen);
int __settime (const struct timeval *tp);
//dead int __shutdown (int handle, int how);
int __sigaction (int _sig, const struct sigaction *_iact,
    struct sigaction *_oact);
void (*__signal (int sig, void (*handler)()))(int sig);
int __sigpending (sigset_t *_set);
int __sigprocmask (int _how, const sigset_t *_iset, sigset_t *_oset);
int __sigsuspend (const sigset_t *_mask);
unsigned __sleep (unsigned sec);
unsigned __sleep2 (unsigned millisec);
//dead int __socket (int domain, int type, int protocol);
int __spawnve (struct _new_proc *np);
//dead int __stat (const char *name, struct stat *buffer);
int __swchar (int flag, int new_char);
int __uflags (int mask, int new_flags);
long __ulimit (int cmd, long new_limit);
//dead int __umask (int pmode);
void __unwind2 (void *xcpt_reg_ptr);
int __utimes (const char *name, const struct timeval *tvp);
int __ttyname (int handle, char *buf, size_t buf_size);
int __wait (int *status);
int __waitpid (int pid, int *status, int options);
int __write (int handle, const void *buf, size_t nbyte);

#if defined (__cplusplus)
}
#endif

#endif /* not _EMX_SYSCALLS_H */
