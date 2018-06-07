/* sys/timeb.h (emx+gcc) */

#ifndef _SYS_TIMEB_H
#define _SYS_TIMEB_H

#if defined (__cplusplus)
extern "C" {
#endif

/** @todo fixme, I'm signed now! */
#if !defined (_TIME_T)
#define _TIME_T
typedef unsigned long time_t;
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
