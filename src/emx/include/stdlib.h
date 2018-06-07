/* stdlib.h,v 1.24 2004/09/14 22:27:36 bird Exp */
/** @file
 * FreeBSD 5.3
 * @changed bird: EMXifications and OS2ifications.
 */

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)stdlib.h	8.5 (Berkeley) 5/19/95
 * $FreeBSD: src/include/stdlib.h,v 1.56 2004/02/23 03:16:58 ache Exp $
 */

#ifndef _STDLIB_H_
#define	_STDLIB_H_

#include <sys/cdefs.h>
#include <sys/_null.h>
#include <sys/_types.h>

#if __BSD_VISIBLE
#ifndef _RUNE_T_DECLARED
typedef	__rune_t	rune_t;
#define	_RUNE_T_DECLARED
#endif
#endif

#if !defined(_SIZE_T_DECLARED) && !defined(_SIZE_T) /* bird: emx */
typedef	__size_t	size_t;
#define	_SIZE_T_DECLARED
#define	_SIZE_T                         /* bird: emx */
#endif

#ifndef	__cplusplus
#if !defined(_WCHAR_T_DECLARED) && !defined(_WCHAR_T) /* bird: emx */
typedef	__wchar_t	wchar_t;
#define	_WCHAR_T_DECLARED
#define	_WCHAR_T                        /* bird: emx */
#endif
#endif

typedef struct _div_t {                 /* bird: emx (tag) */
	int	quot;		/* quotient */
	int	rem;		/* remainder */
} div_t;

typedef struct _ldiv_t {                /* bird: emx (tag) */
	long	quot;
	long	rem;
} ldiv_t;

#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0

#define	RAND_MAX	0x7fffffff

extern int __mb_cur_max;
#define	MB_CUR_MAX	__mb_cur_max

__BEGIN_DECLS
void	 abort(void) __dead2;
int	 abs(int) __pure2;
int	 atexit(void (*)(void));
double	 atof(const char *);
int	 atoi(const char *);
long	 atol(const char *);
void	*bsearch(const void *, const void *, size_t,
	    size_t, int (*)(const void *, const void *));
void	*calloc(size_t, size_t);
div_t	 div(int, int) __pure2;
void	 exit(int) __dead2;
void	 free(void *);
char	*getenv(const char *);
long	 labs(long) __pure2;
ldiv_t	 ldiv(long, long) __pure2;
void	*malloc(size_t);
int	 mblen(const char *, size_t);
size_t	 mbstowcs(wchar_t * __restrict , const char * __restrict, size_t);
int	 mbtowc(wchar_t * __restrict, const char * __restrict, size_t);
void	 qsort(void *, size_t, size_t,
	    int (*)(const void *, const void *));
int	 rand(void);
void	*realloc(void *, size_t);
void	 srand(unsigned);
double	 strtod(const char * __restrict, char ** __restrict);
float	 strtof(const char * __restrict, char ** __restrict);
long	 strtol(const char * __restrict, char ** __restrict, int);
long double
	 strtold(const char * __restrict, char ** __restrict);
unsigned long
	 strtoul(const char * __restrict, char ** __restrict, int);
int	 system(const char *);
int	 wctomb(char *, wchar_t);
size_t	 wcstombs(char * __restrict, const wchar_t * __restrict, size_t);

/*
 * Functions added in C99 which we make conditionally available in the
 * BSD^C89 namespace if the compiler supports `long long'.
 * The #if test is more complicated than it ought to be because
 * __BSD_VISIBLE implies __ISO_C_VISIBLE == 1999 *even if* `long long'
 * is not supported in the compilation environment (which therefore means
 * that it can't really be ISO C99).
 *
 * (The only other extension made by C99 in thie header is _Exit().)
 */
#if __ISO_C_VISIBLE >= 1999
#ifdef __LONG_LONG_SUPPORTED
/* LONGLONG */
typedef struct _lldiv_t {               /* bird: emx (tag) */
	long long quot;
	long long rem;
} lldiv_t;

/* LONGLONG */
long long
	 atoll(const char *);
/* LONGLONG */
long long
	 llabs(long long) __pure2;
/* LONGLONG */
lldiv_t	 lldiv(long long, long long) __pure2;
/* LONGLONG */
long long
	 strtoll(const char * __restrict, char ** __restrict, int);
/* LONGLONG */
unsigned long long
	 strtoull(const char * __restrict, char ** __restrict, int);
#endif /* __LONG_LONG_SUPPORTED */

void	 _Exit(int) __dead2;
#endif /* __ISO_C_VISIBLE >= 1999 */

/*
 * Extensions made by POSIX relative to C.  We don't know yet which edition
 * of POSIX made these extensions, so assume they've always been there until
 * research can be done.
 */
#if __POSIX_VISIBLE /* >= ??? */
int	 posix_memalign(void **, size_t, size_t); /* bird: we implement this. */
int	 rand_r(unsigned *);			/* (TSF) */
int	 setenv(const char *, const char *, int);
int	 unsetenv(const char *);        /* bird: standard saith shall return int. */
#endif

/*
 * The only changes to the XSI namespace in revision 6 were the deletion
 * of the ttyslot() and valloc() functions, which FreeBSD never declared
 * in this header.  For revision 7, ecvt(), fcvt(), and gcvt(), which
 * FreeBSD also does not have, and mktemp(), are to be deleted.
 */
#if __XSI_VISIBLE
/* XXX XSI requires pollution from <sys/wait.h> here.  We'd rather not. */
/* long	 a64l(const char *); */
double	 drand48(void);
/* char	*ecvt(double, int, int * __restrict, int * __restrict); */
double	 erand48(unsigned short[3]);
/* char	*fcvt(double, int, int * __restrict, int * __restrict); */
/* char	*gcvt(double, int, int * __restrict, int * __restrict); */
int	 getsubopt(char **, char *const *, char **);
/** @todo int	 grantpt(int); */
char	*initstate(unsigned long /* XSI requires u_int */, char *, long);
long	 jrand48(unsigned short[3]);
/* char	*l64a(long); */
void	 lcong48(unsigned short[7]);
long	 lrand48(void);
#ifndef _MKSTEMP_DECLARED
int	 mkstemp(char *);
#define	_MKSTEMP_DECLARED
#endif
#ifndef _MKTEMP_DECLARED
char	*mktemp(char *);
#define	_MKTEMP_DECLARED
#endif
long	 mrand48(void);
long	 nrand48(unsigned short[3]);
/** @todo int	 posix_openpt(int); */
/** @todo char	*ptsname(int); */
int	 putenv(const char *);
long	 random(void);
char	*realpath(const char *, char resolved_path[]);
unsigned short
	*seed48(unsigned short[3]);
#ifndef _SETKEY_DECLARED
/** @todo int	 setkey(const char *); */
#define	_SETKEY_DECLARED
#endif
char	*setstate(/* const */ char *);
void	 srand48(long);
void	 srandom(unsigned long);
/** @todo int	 unlockpt(int); */
#endif /* __XSI_VISIBLE */

#if __BSD_VISIBLE
extern const char *_malloc_options;
extern void (*_malloc_message)(const char *, const char *, const char *,
	    const char *);

/*
 * The alloca() function can't be implemented in C, and on some
 * platforms it can't be implemented at all as a callable function.
 * The GNU C compiler provides a built-in alloca() which we can use;
 * in all other cases, provide a prototype, mainly to pacify various
 * incarnations of lint.  On platforms where alloca() is not in libc,
 * programs which use it will fail to link when compiled with non-GNU
 * compilers.
 */
#if __GNUC__ >= 2 || defined(__INTEL_COMPILER)
#undef  alloca	/* some GNU bits try to get cute and define this on their own */
#define alloca(sz) __builtin_alloca(sz)
#elif defined(lint)
void	*alloca(size_t);
#endif

__uint32_t
	 arc4random(void);
void	 arc4random_addrandom(unsigned char *dat, int datlen);
void	 arc4random_stir(void);
char	*getbsize(int *, long *);
					/* getcap(3) functions */
/** @todo char	*cgetcap(char *, const char *, int); */
/** @todo int	 cgetclose(void); */
/** @todo int	 cgetent(char **, char **, const char *); */
/** @todo int	 cgetfirst(char **, char **); */
/** @todo int	 cgetmatch(const char *, const char *); */
/** @todo int	 cgetnext(char **, char **); */
/** @todo int	 cgetnum(char *, const char *, long *); */
/** @todo int	 cgetset(const char *); */
/** @todo int	 cgetstr(char *, const char *, char **); */
/** @todo int	 cgetustr(char *, const char *, char **); */

/** @todo int	 daemon(int, int); */
/** @todo char	*devname(int, int); */
/** @todo char 	*devname_r(int, int, char *, int); */
int	 getloadavg(double [], int);
__const char *
	 getprogname(void);

int	 heapsort(void *, size_t, size_t, int (*)(const void *, const void *));
int	 mergesort(void *, size_t, size_t, int (*)(const void *, const void *));
void	 qsort_r(void *, size_t, size_t, void *,
	    int (*)(void *, const void *, const void *));
int	 radixsort(const unsigned char **, int, const unsigned char *,
	    unsigned);
void    *reallocf(void *, size_t);
void	 setprogname(const char *);
int	 sradixsort(const unsigned char **, int, const unsigned char *,
	    unsigned);
void	 sranddev(void);
void	 srandomdev(void);

/* Deprecated interfaces, to be removed in FreeBSD 6.0. */
/** @todo __int64_t
	 strtoq(const char *, char **, int); */
/** @todo __uint64_t
	 strtouq(const char *, char **, int); */

extern char *suboptarg;			/* getsubopt(3) external variable */
#endif /* __BSD_VISIBLE */



/* bird: LIBC stuff - start  */
#ifdef __BSD_VISIBLE
char    *_getcwdux(char *, size_t);
char    *_realrealpath(const char *, char *, size_t);
int     _getenv_longlong(const char *, long long *);
int     _getenv_long(const char *, long *);
int     _getenv_int(const char *, int *);
#endif
#ifdef  __USE_GNU
char    *canonicalize_file_name(const char *);
void    *valloc(size_t);
#endif
int      on_exit(void (*)(int, void *), void *);
/* bird: LIBC stuff - end  */


/* bird: EMX/PC stuff - start  */

#include <malloc.h>

#if !defined (_MAX_PATH)
#define _MAX_PATH   260
#define _MAX_DRIVE    3
#define _MAX_DIR    256
#define _MAX_FNAME  256
#define _MAX_EXT    256
#endif

#if !defined (__STRICT_ANSI__) && !defined (_POSIX_SOURCE) || defined(__USE_EMX)

#if !defined (OS2_MODE)
#define DOS_MODE 0
#define OS2_MODE 1
#endif

#ifndef _INTPTR_T_DECLARED
typedef	__intptr_t	intptr_t;
typedef	__uintptr_t	uintptr_t;
#define	_INTPTR_T_DECLARED
#endif

#if !defined(_MODE_T_DECLARED) && !defined(_MODE_T) /* bird: emx */
typedef	__mode_t	mode_t;
#define	_MODE_T_DECLARED
#define _MODE_T                         /* bird: emx */
#endif

/* declare _errno to be compatible with MS/VAC/WATCOM */
#if !defined (_ERRNO)
#define _ERRNO
extern int * _errno(void);
#define errno (* _errno())
#endif /* bird: emx */

#if !defined (_ULDIV_T)
#define _ULDIV_T
typedef struct
{
  unsigned long quot;
  unsigned long rem;
} _uldiv_t;

#if __ISO_C_VISIBLE >= 1999 && defined(__LONG_LONG_SUPPORTED)
typedef struct _lldiv_t _lldiv_t;
#else
typedef struct
{
  long long quot;
  long long rem;
} _lldiv_t;
#endif
typedef struct
{
  unsigned long long quot;
  unsigned long long rem;
} _ulldiv_t;
#endif

extern char **environ;

extern const unsigned char _osminor;
extern const unsigned char _osmajor;

/* No DOS support ... */
#define _osmode OS2_MODE

unsigned alarm (unsigned);
int brk(const void *);
int chdir (const char *);
char *gcvt (double, int, char *); /* this is rubbish standardwise */
char *getcwd (char *, size_t);
int getpagesize (void);
char *getwd (char *);
int	mkdir(const char *, mode_t);
void perror (const char *);
int rmdir (const char *);
void *sbrk(intptr_t);
unsigned sleep (unsigned);
long ulimit (int, ...);

char *itoa (int, char *, int);
char *ltoa (long, char *, int);
char *ultoa (unsigned long, char *, int);
char *lltoa (long long, char *, int);
char *ulltoa (unsigned long long, char *, int);

#endif


#if (!defined (__STRICT_ANSI__) && !defined (_POSIX_SOURCE)) || defined (_WITH_UNDERSCORE) || defined(__USE_EMX)

extern char **_environ;
extern const char * const _sys_errlist[];
extern const int _sys_nerr;

unsigned _alarm (unsigned);
int _brk(const void *);
int _chdir (const char *);
char *_gcvt (double, int, char *);
char *_getcwd (char *, size_t);
int _getpagesize (void);
char *_getwd (char *);
int _mkdir (const char *, long);
int _putenv (const char *);
int _rmdir (const char *);
void *_sbrk(intptr_t);
unsigned _sleep (unsigned);
void _swab (const void *, void *, size_t);
long _ulimit (int, ...);

int _abspath (char *, const char *, int);
long long _atoll (const char *);
long double _atofl (const char *);
int _chdir2 (const char *);
int _chdir_os2 (const char *);
int _chdrive (int);
int _core (int);
void _defext (char *, const char *);
void _envargs (int *, char ***, const char *);
int _execname (char *, size_t);
void _exit (int) __attribute__ ((__noreturn__));
int _filesys (const char *, char *, size_t);
char **_fnexplode (const char *);
void _fnexplodefree (char **);
char _fngetdrive (const char *);
int _fnisabs (const char *);
int _fnisrel (const char *);
void _fnlwr (char *);
void _fnlwr2 (char *, const char *);
char *_fnslashify (char *);
int _fullpath (char *, const char *, int);
int _getcwd1 (char *, char);
char *_getcwd2 (char *, int);
int _getdrive (void);
char *_getext (const char *);
char *_getext2 (const char *);
char *_getname (const char *);
int _getsockhandle (int);
int _gettid (void);
char *_getvol (char);
char *_itoa (int, char *, int);
_lldiv_t _lldiv (long long, long long);
char *_lltoa (long long, char *, int);
_uldiv_t _uldiv (unsigned long, unsigned long);
_ulldiv_t _ulldiv (unsigned long long, unsigned long long);
char *_itoa (int, char *, int);
char *_ltoa (long, char *, int);
char *_ultoa (unsigned long, char *, int);
char *_lltoa (long long, char *, int);
char *_ulltoa (unsigned long long, char *, int);
void _makepath (char *, const char *, const char *,
    const char *, const char *);
int _path (char *, const char *);
int _path2(const char *pszName, const char *pszSuffixes, char *pszDst, size_t cbDst);
int _read_kbd (int, int, int);
void _remext (char *);
void _rfnlwr (void);
void _response (int *, char ***);
void _scrsize (int *);
void _searchenv (const char *, const char *, char *);
int _searchenv2(const char *pszEnvVar, const char *pszName, unsigned fFlags, const char *pszSuffixes,
                char *pszDst, size_t cbDst);
int _searchenv2_value(const char *pszSearchPath, const char *pszName, unsigned fFlags, const char *pszSuffixes,
                      char *pszDst, size_t cbDst);
int _searchenv2_has_suffix(const char *pszName, size_t cchName, const char *pszSuffixes);
int _searchenv2_one_file(char *pszDst, size_t cbDst, size_t cchName, unsigned fFlags, const char *pszSuffixes);
/** @name _SEARCHENV2_F_XXX - Flags for _searchenv2(), _searchenv2_value()
 *                            and _searchenv2_one_file().
 * @{ */
/** Indicates that we're searching for an executable file. */
#define _SEARCHENV2_F_EXEC_FILE     1
/** Indicates that we shouldn't explicitly check the current directory before
 * searching the search path. */
#define _SEARCHENV2_F_SKIP_CURDIR   2
/** @} */

void _sfnlwr (const char *);
unsigned _sleep2 (unsigned);
char ** _splitargs (char *, int *);
void _splitpath (const char *, char *, char *, char *, char *);
float _strtof (const char *, char **);
long double _strtold (const char *, char **);
long long _strtoll (const char *, char **, int);
unsigned long long _strtoull (const char *, char **, int);
char _swchar (void);
void _wildcard (int *, char ***);

int _beginthread (void (*)(void *), void *, unsigned, void *);
void _endthread (void);
void **_threadstore (void);

int _setenv(const char *envname, const char *envval, int overwrite);
int _unsetenv(const char *name);

#endif

__END_DECLS

/* bird: EMX/PC stuff - end */

#endif /* !_STDLIB_H_ */

