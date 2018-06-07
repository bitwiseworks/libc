/* sys/select.h (emx+gcc) */

#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H
#define _SYS_SELECT_H_

#if defined (__cplusplus)
extern "C" {
#endif

#include <sys/cdefs.h>
#include <sys/_sigset.h>

#ifndef _SIGSET_T_DECLARED
#define _SIGSET_T_DECLARED
/** Signal set. */
typedef __sigset_t sigset_t;
#endif

/** Define the number of file handles in the select buffer.
 * @remark we might wanna bump this one up a bit... */
#if !defined (FD_SETSIZE)
#define	FD_SETSIZE 2048
#elif FD_SETSIZE < 256
#error FD_SETSIZE must be at least 256
#endif

/** BSD thingy to figure out the size of a bitmap array. */
#ifndef _howmany
#define	_howmany(a,b)       (((a) + ((b) - 1)) / (b))
#endif
#if defined(TCPV40HDRS) && !defined(howmany)
#define	howmany(a,b)        (((a) + ((b) - 1)) / (b))
#endif

#if !defined (_FD_SET_T)
#define _FD_SET_T
/** The base type for the select file descriptor bitmap. */
typedef unsigned long   __fd_mask;
/** Number of bits in a byte. */
#define NBBY        8
/** Number of bits in a byte. */
#define _NFDBITS    (sizeof(__fd_mask) * 8)	/* bits per mask */
/** Select set. */
typedef struct fd_set
{
    __fd_mask   __fds_bits[_howmany(FD_SETSIZE, _NFDBITS)];
} fd_set;

#if defined(__BSD_VISIBLE) || defined(TCPV40HDRS)
typedef __fd_mask   fd_mask;
#define fds_bits    __fds_bits
#define NFDBITS     _NFDBITS
#endif

#endif

#ifndef FD_SET
/** Set a bit in the select file descriptor bitmap. */
#define	FD_SET(n,s)    ((s)->__fds_bits[(n)/_NFDBITS] |=  (1L << ((n) & (_NFDBITS - 1))))
/** Clear a bit in the select file descriptor bitmap. */
#define	FD_CLR(n,s)    ((s)->__fds_bits[(n)/_NFDBITS] &= ~(1L << ((n) & (_NFDBITS - 1))))
/** Test if a bit in the select file descriptor bitmap is set. */
#define	FD_ISSET(n,s)  ((s)->__fds_bits[(n)/_NFDBITS] &   (1L << ((n) & (_NFDBITS - 1))))
/** Initialize the select file descriptor bitmap clearing all bits. */
#define FD_ZERO(s)     (void)memset(s, 0, sizeof(*(s)))
#if __BSD_VISIBLE
/** Copy a select file descriptor bitmap. */
#define	FD_COPY(src,trg) (void)(*(trg) = *(src))
#endif
#endif /* !FD_SET */

/* bird: baka baka! need memset prototype which needs size_t. */
#include <sys/_types.h>
#if !defined(_SIZE_T_DECLARED) && !defined(_SIZE_T) /* bird: emx */
typedef	__size_t	size_t;
#define	_SIZE_T_DECLARED
#define _SIZE_T                         /* bird: emx */
#endif

struct timeval;
int select (int, struct fd_set *, struct fd_set *, struct fd_set *, struct timeval *);
int _select (int, struct fd_set *, struct fd_set *, struct fd_set *, struct timeval *);
void *memset (void *, int, size_t); /* Used by FD_ZERO */
#ifdef _LIBC_TODO
/** A slightly different select call which have better time precision and signal support. */
#warning int pselect(int, fd_set *, fd_set *, fd_set *, const struct timespec *, const sigset_t *);
#endif


/** This is the bsd styled select.
 * Normally it's mapped to select(), but since we don't want any such
 * confusion we don't.
 * @remark The normal select() doesn't have socket capabilities since the sys style libc.
 */
int _System bsdselect(int, struct fd_set *, struct fd_set *, struct fd_set *, struct timeval *);
/** This is the TCPIP OS/2 styled select. */
int _System os2_select(int *, int, int, int, long);

/* toolkit BSD pollution: */
#ifndef TCPV40HDRS
#include <sys/_types.h>
#if !defined(_PID_T_DECLARED) && !defined(_PID_T) /* bird:emx */
typedef	__pid_t		pid_t;		/* process id */
#define	_PID_T_DECLARED
#define _PID_T                          /* bird: emx */
#endif

/*
 * Used to maintain information about processes that wish to be
 * notified when I/O becomes possible.
 */
#pragma pack(1)
struct selinfo {
        pid_t   si_pid;         /* process to be notified */
        short   si_flags;       /* see below */
};
#pragma pack()
#define SI_COLL 0x0001          /* collision occurred */
#endif /* !TCPV40HDRS */

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_SELECT_H */
