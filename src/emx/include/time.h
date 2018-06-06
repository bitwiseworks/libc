/** @file
 * FreeBSD 5.1
 *
 * @changed EMX isms.
 * @changed Removed timezone() function as it clashes with the timezone
 *          variable defined in EMX and OGBSI6.
 * @changed Changed CLK_TCK and CLOCKS_PER_SEC to 100.
 * @changed Commented out tm_gmtoff and tm_zone in struct tm as LIBC doesn't
 *          implement these yet.
 *
 * @todo    Implement tm_gmtoff and tm_zone members in struct tm.
 * @todo    Implement clock_getres(), clock_gettime(), clock_settime() which 
 *          are defined in various POSIX standard revisions.
 * @todo    Implement tzsetwall(), timelocal(), timegm() which are BSD specific.
 */
/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)time.h	8.3 (Berkeley) 1/21/94
 */

/*
 * $FreeBSD: src/include/time.h,v 1.30 2002/09/06 11:23:32 tjr Exp $
 */

#ifndef _TIME_H_
#define	_TIME_H_
#define	_TIME_H                         /* bird: EMX */

#include <sys/cdefs.h>
#include <sys/_types.h>

#if __POSIX_VISIBLE > 0 && __POSIX_VISIBLE < 200112 || __BSD_VISIBLE
/*
 * Frequency of the clock ticks reported by times().  Deprecated - use
 * sysconf(_SC_CLK_TCK) instead.  (Removed in 1003.1-2001.)
 */
#define CLK_TCK         100             /* bird: EMX uses 100, FreeBSD 128. */
#endif

/* Frequency of the clock ticks reported by clock().  */
#define CLOCKS_PER_SEC  100             /* bird: EMX uses 100, FreeBSD 128. */

#ifndef	NULL
#define	NULL	0
#endif

#if !defined(_CLOCK_T_DECLARED) && !defined(_CLOCK_T) /* bird: EMX */
typedef	__clock_t	clock_t;
#define	_CLOCK_T_DECLARED
#define _CLOCK_T                        /* bird: EMX */
#endif

#if !defined(_TIME_T_DECLARED) && !defined(_TIME_T) /* bird: EMX */
typedef	__time_t	time_t;
#define	_TIME_T_DECLARED
#define _TIME_T                         /* bird: EMX */
#endif

#ifndef _TIME64_T_DECLARED              /* bird: LIBC */
typedef	__int64_t	time64_t;       /* bird: LIBC */
#define	_TIME64_T_DECLARED              /* bird: LIBC */
#endif                                  /* bird: LIBC */

#if !defined(_SIZE_T_DECLARED) && !defined(_SIZE_T) /* bird: EMX */
typedef	__size_t	size_t;
#define	_SIZE_T_DECLARED
#define _SIZE_T                         /* bird: EMX */
#endif

#if __POSIX_VISIBLE >= 199309
/*
 * New in POSIX 1003.1b-1993.
 */
#if !defined(_CLOCKID_T_DECLARED) && !defined(_CLOCKID_T) /* bird: EMX */
typedef	__clockid_t	clockid_t;
#define	_CLOCKID_T_DECLARED
#define _CLOCKID_T                      /* bird: EMX */
#endif

#if !defined(_TIMER_T_DECLARED) && !defined(_TIMER_T) /* bird: EMX */
typedef	__timer_t	timer_t;
#define	_TIMER_T_DECLARED
#define _TIMER_T                        /* bird: EMX */
#endif

#include <sys/timespec.h>
#endif /* __POSIX_VISIBLE >= 199309 */

struct tm {
	int	tm_sec;		/* seconds after the minute [0-60] */
	int	tm_min;		/* minutes after the hour [0-59] */
	int	tm_hour;	/* hours since midnight [0-23] */
	int	tm_mday;	/* day of the month [1-31] */
	int	tm_mon;		/* months since January [0-11] */
	int	tm_year;	/* years since 1900 */
	int	tm_wday;	/* days since Sunday [0-6] */
	int	tm_yday;	/* days since January 1 [0-365] */
	int	tm_isdst;	/* Daylight Savings Time flag */
#if 0 /* bird: LIBC isn't implementing tm_gmtoff and tm_zone. */
	long	tm_gmtoff;	/* offset from UTC in seconds */
	char	*tm_zone;	/* timezone abbreviation */
#endif
};

#if __POSIX_VISIBLE
extern char *tzname[];
#endif

__BEGIN_DECLS
char *asctime(const struct tm *);
clock_t clock(void);
char *ctime(const time_t *);
double difftime(time_t, time_t);
struct tm *gmtime(const time_t *);
struct tm *localtime(const time_t *);
struct tm *_localtime64_r(const time64_t *, struct tm *); /* bird: LIBC */
time_t mktime(struct tm *);
time64_t _mktime64(struct tm *); /* bird: LIBC  */
size_t strftime(char * __restrict, size_t, const char * __restrict,
    const struct tm * __restrict);
time_t time(time_t *);

#if __POSIX_VISIBLE
void tzset(void);
#endif

#if __POSIX_VISIBLE >= 199309
int clock_getres(clockid_t, struct timespec *);
int clock_gettime(clockid_t, struct timespec *);
int clock_settime(clockid_t, const struct timespec *);
int nanosleep(const struct timespec *, struct timespec *);
#endif /* __POSIX_VISIBLE >= 199309 */

#if __POSIX_VISIBLE >= 199506
char *asctime_r(const struct tm *, char *);
char *ctime_r(const time_t *, char *);
struct tm *gmtime_r(const time_t *, struct tm *);
struct tm *_gmtime64_r(const time64_t *, struct tm *); /* bird: LIBC */
struct tm *localtime_r(const time_t *, struct tm *);
#endif

#if __XSI_VISIBLE
char *strptime(const char * __restrict, const char * __restrict,
    struct tm * __restrict);
#endif

#if __BSD_VISIBLE
/* bird: clash with EMX (and OGBSI6) timezone is a variable. man page say this
 * is for Unix version 7 compatability though...
char *timezone(int, int); */
void tzsetwall(void);
time_t timelocal(struct tm * const);
time_t timegm(struct tm * const);
#endif /* __BSD_VISIBLE */

/* bird: LIBC OGBSI6 compliance extras. */
#if __POSIX_VISIBLE
extern int daylight;
extern long timezone;
#endif

/* bird: EMX/VAC/MSC legacy start. */
#if (!defined (__STRICT_ANSI__) && !defined (_POSIX_SOURCE)) || defined (_WITH_UNDERSCORE) || defined(__USE_EMX)
extern int _daylight;
extern long _timezone;
extern char *_tzname[2];
char *_strptime (const char *, const char *, struct tm *);
void _tzset (void);
#endif
/* bird: EMX stuff end. */

__END_DECLS

#endif /* !_TIME_H_ */

