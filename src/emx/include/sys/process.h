/* $Id: $ */
/** @file
 * EMX
 */

#ifndef _SYS_PROCESS_H
#define _SYS_PROCESS_H

#include <sys/cdefs.h>
#include <sys/_types.h>

__BEGIN_DECLS

#if !defined(_PID_T_DECLARED) && !defined(_PID_T)
typedef	__pid_t pid_t;
#define	_PID_T_DECLARED
#define _PID_T
#endif

#if !defined (P_WAIT)
#define P_WAIT    0
#define P_NOWAIT  1
#define P_OVERLAY 2
#define P_DEBUG   3
#define P_SESSION 4
#define P_DETACH  5
#define P_PM      6

#define P_DEFAULT    0x0000
#define P_MINIMIZE   0x0100
#define P_MAXIMIZE   0x0200
#define P_FULLSCREEN 0x0300
#define P_WINDOWED   0x0400

#define P_FOREGROUND 0x0000
#define P_BACKGROUND 0x1000

#define P_NOCLOSE    0x2000
#define P_NOSESSION  0x4000

#define P_QUOTE      0x10000
#define P_TILDE      0x20000
#define P_UNRELATED  0x40000
#define P_DEBUGDESC  0x80000
#define P_NOUNIXARGV 0x100000

#endif

void abort (void)  __attribute__ ((__noreturn__));
int atexit (void (*)(void));
int execl (const char *, const char *, ...);
int execle (const char *, const char *, ...);
int execlp (const char *, const char *, ...);
int execlpe (const char *, const char *, ...);
int execv (const char *, char * const *);
int execve (const char *, char * const *, char * const *);
int execvp (const char *, char * const *);
int execvpe (const char *, char * const *, char * const *);
void exit (int) __attribute__ ((__noreturn__));
void _exit (int) __attribute__ ((__noreturn__));
pid_t fork (void);
pid_t getpid (void);
pid_t getppid (void);
int spawnl (int, const char *, const char *, ...);
int spawnle (int, const char *, const char *, ...);
int spawnlp (int, const char *, const char *, ...);
int spawnlpe (int, const char *, const char *, ...);
int spawnv (int, const char *, char * const *);
int spawnve (int, const char *, char * const *, char * const *);
int spawnvp (int, const char *, char * const *);
int spawnvpe (int, const char *, char * const *, char * const *);
int system (const char *);
pid_t wait (int *);
pid_t waitpid (pid_t, int *, int);

int _execl (const char *, const char *, ...);
int _execle (const char *, const char *, ...);
int _execlp (const char *, const char *, ...);
int _execlpe (const char *, const char *, ...);
int _execv (const char *, char * const *);
int _execve (const char *, char * const *, char * const *);
int _execvp (const char *, char * const *);
int _execvpe (const char *, char * const *, char * const *);
int _fork (void);
pid_t _getpid (void);
pid_t _getppid (void);
int _spawnl (int, const char *, const char *, ...);
int _spawnle (int, const char *, const char *, ...);
int _spawnlp (int, const char *, const char *, ...);
int _spawnlpe (int, const char *, const char *, ...);
int _spawnv (int, const char *, char * const *);
int _spawnve (int, const char *, char * const *, char * const *);
int _spawnvp (int, const char *, char * const *);
int _spawnvpe (int, const char *, char * const *, char * const *);
pid_t _wait (int *);
pid_t _waitpid (pid_t, int *, int);

__END_DECLS

#endif /* not _SYS_PROCESS_H */
