/* tzset.c (emx+gcc) -- Copyright (c) 1992-1996 by Kai Uwe Rommel */
/*                      Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <emx/time.h>
#include <emx/syscalls.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

int _STD(daylight) = 0;
long _STD(timezone) = 0;
char *_STD(tzname)[2] = {"UCT", ""};

struct _tzinfo _tzi = {"UCT", "", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


/* Parse a decimal number.  Return a non-zero value if successful,
   zero on error. */

static int dnum (int *dst, char **pp, int min, int max, int opt_sign,
                 int delim)
{
  char *p;
  int n, sign, amax;

  p = *pp; n = 0; sign = 1;

  /* Optional sign. */

  if (opt_sign)
    {
      if (*p == '+')
        ++p;
      else if (*p == '-')
        sign = -1, ++p;
    }

  /* There must be at least one digit. */

  if (!(*p >= '0' && *p <= '9'))
    return 0;

  /* Compute the maximum absolute value of the number.  This is used
     to detect overflow. */

  amax = max;
  if (-min > amax) amax = -min;
  do
    {
      n = n * 10 + *p - '0';
      if (n > amax) return 0;   /* Overflow */
      ++p;
    } while (*p >= '0' && *p <= '9');

  /* Check the delimiter if DELIM is not -1. */

  if (delim != -1 && *p++ != delim)
    return 0;

  /* Apply the sign. */

  if (sign == -1)
    n = -n;

  /* Check the range. */

  if (n < min || n > max)
    return 0;

  /* Return the result. */

  *dst = n;
  *pp = p;
  return 1;
}


/* Copy a timezone name.  Return a non-zero value if successful, zero
   on error. */

static int copy_tzname (char *dst, char **pp)
{
  char *p;
  int i;

  p = *pp;
  i = 0;

  /* Skip the characters making up base timezone name */
  while ((p [i] >= 'A' && p [i] <= 'Z') || (p [i] >= 'a' && p [i] <= 'z'))
    i++;

  *pp = p + i;
  if (i > __MAX_TZ_STANDARD_LEN)
    i = __MAX_TZ_STANDARD_LEN;
  memcpy (dst, p, i);
  dst [i] = 0;
  return 1;
}

static int parse_delta (char **src, int *offset, int opt_sign)
{
  int ofs = 0, sign = 1, temp;

  if (!dnum (&ofs, src, -23, 23, opt_sign, -1))
    return 0;
  if (ofs < 0)
    sign = -1, ofs = -ofs;

  ofs *= 60;
  if (**src == ':')            /* Minutes specified? */
    {
      (*src)++;
      if (!dnum (&temp, src, 0, 59, 0, -1))
        return 0;
      ofs += temp;
    }

  ofs *= 60;
  if (**src == ':')            /* Seconds specified? */
    {
      (*src)++;
      if (!dnum (&temp, src, 0, 59, 0, -1))
        return 0;
      ofs += temp;
    }

  *offset = sign > 0 ? ofs : -ofs;

  return 1;
}

static int parse_switchtime (char **src, int *m, int *w, int *d, int *t)
{
  if (!dnum (m, src, 1, 12, 0, ',')
   || !dnum (w, src, -4, 4, 1, ',')
   || !dnum (d, src, *w ? 0 : 1, *w ? 6 : 31, 0, ',')
   || !dnum (t, src, 0, 86399, 0, ','))
    return 0;

  return 1;
}

/* The format of TZ environment variable:
 *
 * TZ1[OFF,[TZ2[,SM,SW,SD,ST,EM,EW,ED,ET,SHIFT]]]
 *
 * TZ1 is the at least three-letter name of the standard timezone.
 *
 * OFF is the offset to Coordinated Universal Time; positive values are to the
 * west of the Prime Meridian, negative values are to the east of the Prime
 * Meridian. The offset has the format [H[:M[:S]]].
 *
 * TZ2 is the three-letter name of the summer timezone (daylight saving time).
 * If TZ2 is not specified, daylight saving time does not apply.
 *
 * SM specifies the month (1 through 12) of the change. SW specifies the week
 * of the change; if this value is zero, SD specifies the day of month
 * (1 through 31). If SW is positive (1 through 4), the change occurs on
 * weekday SD (0=Sunday through 6=Saturday) of the SWth week of the specified
 * month. The first week of a month starts on the first Sunday of the month.
 * If SW is negative (-1 through -4), the change occurs on weekday SD
 * (0=Sunday through 6=Saturday) of the -SWth week of the specified month,
 * counted from the end of the month (that is, -1 specifies the last week of
 * the month). The last week of a month starts on the last Sunday of the
 * month. ST specifies the time of the change, in seconds. Note that ST is
 * specified in local standard time and ET is specified in local daylight
 * saving time.
 *
 * Example:
 *   CET-1CED,3,-1,0,7200,10,-1,0,10800,3600
 *   KWT-4KWST,3,-1,0,7200,10,-1,0,10800,3600
 */
void _STD(tzset) (void)
{
  LIBCLOG_ENTER("\n");
  struct _tzinfo ntz;
  struct timeb tb;
  char *p;
  int offset;
  time_t t_loc;

  p = getenv ("TZ");
  if (p == NULL || *p == 0)
    p = "UCT";                  /* Our best approximation :-) */
  LIBCLOG_MSG("TZ=%s\n", p);

  if (!copy_tzname (ntz.tzname, &p))
    LIBCLOG_ERROR_RETURN_VOID();

  if (*p == 0)
    offset = 0;                 /* TZ=XYZ is valid (in contrast to POSIX.1) */
  else if (!parse_delta (&p, &offset, 1))
    LIBCLOG_ERROR_RETURN_VOID();

  ntz.tz = offset;

  ntz.dst = 0;
  ntz.dstzname[0] = 0;

  if (*p != 0)
    {
      ntz.dst = 1;
      if (!copy_tzname (ntz.dstzname, &p))
        LIBCLOG_ERROR_RETURN_VOID();
      if (*p == ',')
        {
          p++;
          /* Parse DST start date/time */
          if (!parse_switchtime (&p, &ntz.sm, &ntz.sw, &ntz.sd, &ntz.st))
            LIBCLOG_ERROR_RETURN_VOID();
          if (!parse_switchtime (&p, &ntz.em, &ntz.ew, &ntz.ed, &ntz.et))
            LIBCLOG_ERROR_RETURN_VOID();
          if (!dnum (&ntz.shift, &p, 0, 86400, 0, 0))
            LIBCLOG_ERROR_RETURN_VOID();
        }
      else if (*p == 0)
        {
          /* VAC++ default values */
          ntz.sm = 4;  ntz.sw =  1; ntz.sd = 0; ntz.st = 3600;
          ntz.em = 10; ntz.ew = -1; ntz.ed = 0; ntz.et = 7200;
          ntz.shift = 3600;
        }
      else
        {
          /* TODO: POSIX.1 */
          LIBCLOG_ERROR_RETURN_VOID();
        }
      _STD(daylight) = 1;
    }
  else
    _STD(daylight) = 0;


  /* TODO: Make this thread-safe! */
  _tzi = ntz;
  _STD(tzname)[0] = _tzi.tzname;
  _STD(tzname)[1] = _tzi.dstzname;
  _compute_dst_table ();

  __ftime (&tb);
  t_loc = tb.time;
/*  _STD(daylight) = _loc2gmt (&tb.time, -1); */
  _STD(timezone) = _tzi.tz;

  _tzset_flag = 1;
  LIBCLOG_RETURN_MSG_VOID("ret void - _tzi={.tzname=\"%s\", .dstzname=\"%s\" .tz=%d, .dst=%d, .shift=%d, .sm=%d, .sw=%d, .sd=%d, .st=%d, .em=%d, .ew=%d, .ed=%d, .et=%d}\n",
                          ntz.tzname, ntz.dstzname, ntz.tz, ntz.dst, ntz.shift, ntz.sm, ntz.sw, ntz.sd, ntz.st, ntz.em, ntz.ew, ntz.ed, ntz.et);
}
