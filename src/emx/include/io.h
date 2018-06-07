/* io.h,v 1.10 2004/09/14 22:27:33 bird Exp */
/** @file
 * EMX
 */

#ifndef _IO_H
#define _IO_H

#if defined (__cplusplus)
extern "C" {
#endif

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

#if !defined(_OFF_T_DECLARED) && !defined(_OFF_T) /* bird:emx */
typedef	__off_t		off_t;		/* file offset */
#define	_OFF_T_DECLARED
#define _OFF_T                          /* bird: emx */
#endif

#if !defined(_MODE_T_DECLARED) && !defined(_MODE_T)                             /* bird: EMX */
typedef	__mode_t	mode_t;
#define	_MODE_T_DECLARED
#define _MODE_T                                                                 /* bird: EMX */
#endif

#if !defined (SEEK_SET)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

struct stat;
struct fd_set;
struct timeval;

int remove (const char *);
int rename (const char *, const char *);

int access (const char *, int);
int chmod (const char *, mode_t);
int chsize (int, off_t);
int close (int);
int creat (const char *, mode_t);
int dup (int);
int dup2 (int, int);
int eof (int);
off_t filelength (int);
int fstat (int, struct stat *);
int fsync (int);
#ifndef _FTRUNCATE_DECLARED
#define	_FTRUNCATE_DECLARED
int	 ftruncate(int, off_t);
#endif
int ioctl (int, unsigned long request, ...);
int isatty (int);
#ifndef _LSEEK_DECLARED
#define	_LSEEK_DECLARED
off_t	 lseek(int, off_t, int);
#endif
int mkstemp (char *);
char *mktemp (char *);
int open (const char *, int, ...);
int pipe (int *);
ssize_t read (int, void *, size_t);
int select (int, struct fd_set *, struct fd_set *, struct fd_set *, struct timeval *);
int setmode (int, int);
int sopen (const char *, int, int, ...);
int stat (const char *, struct stat *);
off_t tell (int);
#ifndef _TRUNCATE_DECLARED
#define	_TRUNCATE_DECLARED
int	 truncate(const char *, off_t);
#endif
mode_t	umask(mode_t);
int unlink (const char *);
int write (int, const void *, size_t);

#if (!defined (__STRICT_ANSI__) && !defined (_POSIX_SOURCE)) || defined (_WITH_UNDERSCORE) || defined(__USE_EMX)

int _access (const char *, int);
int _chmod (const char *, int);
int _chsize (int, off_t);
int _close (int);
int _creat (const char *, int);
int _crlf (char *, size_t, size_t *);
int _dup (int);
int _dup2 (int, int);
int _eof (int);
off_t _filelength (int);
int _fstat (int, struct stat *);
int _fsync (int);
#ifndef __FTRUNCATE_DECLARED
#define	__FTRUNCATE_DECLARED
int	 _ftruncate(int, off_t);
#endif
int _imphandle (int);
int _ioctl (int, unsigned long request, ...);
int _isatty (int);
int _isterm (int);
#ifndef __LSEEK_DECLARED
#define	__LSEEK_DECLARED
off_t	 _lseek(int, off_t, int);
#endif
int _mkstemp (char *);
char *_mktemp (char *);
int _open (const char *, int, ...);
int _pipe (int *);
ssize_t _read (int, void *, size_t);
int _seek_hdr (int);
int _select (int, struct fd_set *, struct fd_set *, struct fd_set *, struct timeval *);
int _setmode (int, int);
int _sopen (const char *, int, int, ...);
int _stat (const char *, struct stat *);
off_t _tell (int);
int _truncate (char *, off_t);
int _umask (int);
int _unlink (const char *);
int _write (int, const void *, size_t);

#endif

#if defined (__cplusplus)
}
#endif

#endif /* not _IO_H */
