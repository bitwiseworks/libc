/* sys/timeb.h (emx+gcc) */

#ifndef _SYS_TIMEB_H
#define _SYS_TIMEB_H

#include <sys/_types.h>

#if defined (__cplusplus)
extern "C" {
#endif

#if !defined(_TIME_T_DECLARED) && !defined(_TIME_T)
typedef	__time_t time_t;
#define	_TIME_T_DECLARED
#define	_TIME_T
#endif

struct timeb
{
  time_t   time;
  unsigned millitm;
  int      timezone;
  int      dstflag;
};

void ftime (struct timeb *);

void _ftime (struct timeb *);

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_TIMEB_H */
