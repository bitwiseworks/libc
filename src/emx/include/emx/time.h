/* emx/time.h (emx+gcc) */

#ifndef _EMX_TIME_H
#define _EMX_TIME_H

#include <sys/cdefs.h>
#include <sys/_types.h>

#if defined (__cplusplus)
extern "C" {
#endif

#define _YEARS          (2059 - 1900 + 1)
#define TIME_T_MAX      0x7fffffffL
#define TIME_T_MIN      (-0x7fffffffL - 1)
#define TIME64_T_MAX    0x7fffffffffffffffLL
#define TIME64_T_MIN    (-0x7fffffffffffffffLL - 1)

#if !defined(_TIME_T_DECLARED) && !defined(_TIME_T) /* bird: EMX */
typedef	__time_t	time_t;
#define	_TIME_T_DECLARED
#define _TIME_T                         /* bird: EMX */
#endif

#ifndef _TIME64_T_DECLARED              /* bird: LIBC */
typedef	__int64_t	time64_t;       /* bird: LIBC */
#define	_TIME64_T_DECLARED              /* bird: LIBC */
#endif                                  /* bird: LIBC */

struct tm;

/* Maximum length for time zone standard - "GMT", "CEST" etc. */
#define __MAX_TZ_STANDARD_LEN 15

struct _tzinfo
{
  char tzname[__MAX_TZ_STANDARD_LEN + 1];
  char dstzname[__MAX_TZ_STANDARD_LEN + 1];
  int tz, dst, shift;
  int sm, sw, sd, st;
  int em, ew, ed, et;
};

extern int _tzset_flag;
extern struct _tzinfo _tzi;

extern signed short const   _year_day[_YEARS+1];
extern unsigned short const _month_day_leap[];
extern unsigned short const _month_day_non_leap[];


int _day (int, int, int);
int _gmt2loc (time_t *);
int _gmt2loc64 (time64_t *p);
int _loc2gmt (time_t *, int);
int _loc2gmt64 (time64_t *, int);
/* struct tm *_gmtime (struct tm *, const time_t *); - use _gmtime64_r */
/* struct tm *_localtime (struct tm *, const time_t *); - use _localtime64_r */
unsigned long _mktime (struct tm *);
time64_t __mktime64 (struct tm *);
void _compute_dst_table (void);


static __inline__ int _leap_year (unsigned y)
{
  return (y % 4 != 0 ? 0 : y % 100 != 0 ? 1 : y % 400 != 0 ? 0 : 1);
}


#if defined (__cplusplus)
}
#endif

#endif /* not _EMX_TIME_H */
