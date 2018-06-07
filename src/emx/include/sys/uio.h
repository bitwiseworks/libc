/* sys/uio.h (emx+gcc) */

#ifndef _SYS_UIO_H
#define _SYS_UIO_H
#define _SYS_UIO_H_ /*toolkit pollution*/

#if defined (__cplusplus)
extern "C" {
#endif

/* bird: standard saith these shall be definedl */
#include <sys/cdefs.h>
#include <sys/_types.h>

#if !defined(_SIZE_T_DECLARED) && !defined(_SIZE_T) /* bird: emx */
typedef	__size_t	size_t;
#define	_SIZE_T_DECLARED
#define _SIZE_T                         /* bird: emx */
#endif

#if !defined(_SSIZE_T_DECLARED) && !defined(_SSIZE_T) /* bird: emx */
typedef	__ssize_t	ssize_t;
#define	_SSIZE_T_DECLARED
#define _SSIZE_T                        /* bird: emx */
#endif


struct iovec
{
  /* bird: standard saith void* not caddr_t. */
  void *  iov_base;
#if defined(__32BIT__) && !defined(TCPV40HDRS)
  size_t  iov_len;
#else
  int     iov_len;
#endif
};


/* needed for sys\socket.h TCPIPV4 now */
#ifdef TCPV40HDRS
#if !defined(_OFF_T_DECLARED) && !defined(_OFF_T) /* bird:emx */
typedef	__off_t		off_t;		/* file offset */
#define	_OFF_T_DECLARED
#define _OFF_T                          /* bird: emx */
#endif

struct uio {
	struct iovec	*uio_iov;
	int		uio_iovcnt;
	off_t		uio_offset;
	int		uio_segflg;
	unsigned int	uio_resid;
};
#ifndef FREAD
#define FREAD   1
#define FWRITE  2
#endif
#endif

enum	uio_rw { UIO_READ, UIO_WRITE };

#ifndef TCPV40HDRS
/* Segment flag values. */
enum uio_seg {
        UIO_USERSPACE,          /* from user data space */
        UIO_SYSSPACE,           /* from system space */
        UIO_USERISPACE          /* from user I space */
};
#endif

/* TCPIP versions */
int TCPCALL so_readv (int, struct iovec *, int);
int TCPCALL so_writev (int, struct iovec *, int);

/* EMX versions */
int readv (int, const struct iovec *, int);
int writev (int, const struct iovec *, int);

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_UIO_H */
