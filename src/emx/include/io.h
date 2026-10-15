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

#ifndef _REMOVE_DECLARED
#define	_REMOVE_DECLARED
int remove (const char *);
#endif
#ifndef _RENAME_DECLARED
#define	_RENAME_DECLARED
int rename (const char *, const char *);
#endif

#ifndef _ACCESS_DECLARED
#define	_ACCESS_DECLARED
int access (const char *, int);
#endif
#ifndef _CHMOD_DECLARED
#define	_CHMOD_DECLARED
int chmod (const char *, mode_t);
#endif
int chsize (int, off_t);
#ifndef _CLOSE_DECLARED
#define	_CLOSE_DECLARED
int close (int);
#endif
#ifndef _CREAT_DECLARED
#define	_CREAT_DECLARED
int creat (const char *, mode_t);
#endif
#ifndef _DUP_DECLARED
#define	_DUP_DECLARED
int dup (int);
#endif
#ifndef _DUP2_DECLARED
#define	_DUP2_DECLARED
int dup2 (int, int);
#endif
int eof (int);
off_t filelength (int);
#ifndef _FSTAT_DECLARED
#define	_FSTAT_DECLARED
int fstat (int, struct stat *);
#endif
#ifndef _FSYNC_DECLARED
#define	_FSYNC_DECLARED
int fsync (int);
#endif
#ifndef _FTRUNCATE_DECLARED
#define	_FTRUNCATE_DECLARED
int	 ftruncate(int, off_t);
#endif
#ifndef _IOCTL_DECLARED
#define	_IOCTL_DECLARED
int ioctl (int, unsigned long request, ...);
#endif
#ifndef _ISATTY_DECLARED
#define	_ISATTY_DECLARED
int isatty (int);
#endif
#ifndef _LSEEK_DECLARED
#define	_LSEEK_DECLARED
off_t	 lseek(int, off_t, int);
#endif
#ifndef _MKSTEMP_DECLARED
#define	_MKSTEMP_DECLARED
int mkstemp (char *);
#endif
#ifndef _MKTEMP_DECLARED
#define	_MKTEMP_DECLARED
char *mktemp (char *);
#endif
#ifndef _OPEN_DECLARED
#define	_OPEN_DECLARED
int open (const char *, int, ...);
#endif
#ifndef _PIPE_DECLARED
#define	_PIPE_DECLARED
int pipe (int *);
#endif
#ifndef _READ_DECLARED
#define	_READ_DECLARED
ssize_t read (int, void *, size_t);
#endif
#ifndef _SELECT_DECLARED
#define	_SELECT_DECLARED
int select (int, struct fd_set *, struct fd_set *, struct fd_set *, struct timeval *);
#endif
int setmode (int, int);
int sopen (const char *, int, int, ...);
#ifndef _STAT_DECLARED
#define	_STAT_DECLARED
int stat (const char *, struct stat *);
#endif
off_t tell (int);
#ifndef _TRUNCATE_DECLARED
#define	_TRUNCATE_DECLARED
int	 truncate(const char *, off_t);
#endif
#ifndef _UMASK_DECLARED
#define	_UMASK_DECLARED
mode_t	umask(mode_t);
#endif
#ifndef _UNLINK_DECLARED
#define	_UNLINK_DECLARED
int unlink (const char *);
#endif
#ifndef _WRITE_DECLARED
#define	_WRITE_DECLARED
int write (int, const void *, size_t);
#endif

#if (!defined (__STRICT_ANSI__) && !defined (_POSIX_SOURCE)) || defined (_WITH_UNDERSCORE) || defined(__USE_EMX)

#ifndef __ACCESS_DECLARED
#define	__ACCESS_DECLARED
int _access (const char *, int);
#endif
#ifndef __CHMOD_DECLARED
#define	__CHMOD_DECLARED
int _chmod (const char *, int);
#endif
int _chsize (int, off_t);
#ifndef __CLOSE_DECLARED
#define	__CLOSE_DECLARED
int _close (int);
#endif
#ifndef __CREAT_DECLARED
#define	__CREAT_DECLARED
int _creat (const char *, int);
#endif
int _crlf (char *, size_t, size_t *);
#ifndef __DUP_DECLARED
#define	__DUP_DECLARED
int _dup (int);
#endif
#ifndef __DUP2_DECLARED
#define	__DUP2_DECLARED
int _dup2 (int, int);
#endif
int _eof (int);
off_t _filelength (int);
#ifndef __FSTAT_DECLARED
#define	__FSTAT_DECLARED
int _fstat (int, struct stat *);
#endif
int _fsync (int);
#ifndef __FTRUNCATE_DECLARED
#define	__FTRUNCATE_DECLARED
int	 _ftruncate(int, off_t);
#endif
int _imphandle (int);
#ifndef __IOCTL_DECLARED
#define	__IOCTL_DECLARED
int _ioctl (int, unsigned long request, ...);
#endif
#ifndef __ISATTY_DECLARED
#define	__ISATTY_DECLARED
int _isatty (int);
#endif
int _isterm (int);
#ifndef __LSEEK_DECLARED
#define	__LSEEK_DECLARED
off_t	 _lseek(int, off_t, int);
#endif
#ifndef __MKSTEMP_DECLARED
#define	__MKSTEMP_DECLARED
int _mkstemp (char *);
#endif
#ifndef __MKTEMP_DECLARED
#define	__MKTEMP_DECLARED
char *_mktemp (char *);
#endif
#ifndef __OPEN_DECLARED
#define	__OPEN_DECLARED
int _open (const char *, int, ...);
#endif
#ifndef __PIPE_DECLARED
#define	__PIPE_DECLARED
int _pipe (int *);
#endif
#ifndef __READ_DECLARED
#define	__READ_DECLARED
ssize_t _read (int, void *, size_t);
#endif
int _seek_hdr (int);
#ifndef __SELECT_DECLARED
#define	__SELECT_DECLARED
int _select (int, struct fd_set *, struct fd_set *, struct fd_set *, struct timeval *);
#endif
int _setmode (int, int);
int _sopen (const char *, int, int, ...);
#ifndef __STAT_DECLARED
#define	__STAT_DECLARED
int _stat (const char *, struct stat *);
#endif
off_t _tell (int);
int _truncate (char *, off_t);
#ifndef __UMASK_DECLARED
#define	__UMASK_DECLARED
int _umask (int);
#endif
#ifndef __UNLINK_DECLARED
#define	__UNLINK_DECLARED
int _unlink (const char *);
#endif
#ifndef __WRITE_DECLARED
#define	__WRITE_DECLARED
int _write (int, const void *, size_t);
#endif

#endif

#if defined (__cplusplus)
}
#endif

#endif /* not _IO_H */
