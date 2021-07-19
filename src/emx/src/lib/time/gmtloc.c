/* gmtloc.c (emx+gcc) -- Copyright (c) 1995-1996 by Eberhard Mattes */

#include <time.h>
#include <limits.h>
#include <assert.h>
#include <emx/time.h>

/* Return true iff adding A to T does not overflow or underflow time_t.
   (Correct but not fast!!!) */
#define ADD_OK(t,a) \
    ( (t) == 0 || (a) == 0 \
     || ((a) > 0 && (t) > 0 && (a) + (t) > (a)) \
     || ((a) > 0 && (t) < 0 && (a) + (t) < (a)) \
     || ((a) < 0 && (t) < 0 && (a) + (t) < (a)) \
     || ((a) < 0 && (t) > 0 && (a) + (t) > (a)) )

struct _dstswitch
{
  time64_t time;                  /* UTC */
  int shift;
};

static struct _dstswitch _dstsw[2*_YEARS+2] = {{TIME64_T_MIN, 0}, {TIME64_T_MAX, 0}};
static int _dstsw_count = 2;


/* Compute the day of the year on which DST switching occurs. */

static int switch_day (int month, int week, int day, int ywday,
                       const unsigned short *month_table)
{
  int d, wday;

  if (week > 0)
    {
      d = month_table[month-1];
      wday = (d + ywday) % 7;   /* Wekkday of the first day the month */
      if (wday != 0)
        d += 7 - wday;          /* d is the first Sunday of the month */
      d += day;                 /* First DAY in `first week' */
      d += 7 * (week - 1);
    }
  else if (week < 0)
    {
      if (month == 12)
        d = month_table[11] + 31 - 1;
      else
        d = month_table[month] - 1;
      wday = (d + ywday) % 7;   /* Weekday of the last day of the month */
      if (wday != 0)
        d -= wday;              /* d is the last Sunday of the month */
      d += day;                 /* First DAY in `last week' */
      d -= 7 * (week + 1);
    }
  else
    d = month_table[month-1] + day - 1;
  return d;
}


void _compute_dst_table (void)
{
  int y, i, ywday, d_year, d_start, d_end;
  const unsigned short *month_table;
  time64_t t_year, t_start, t_end;

  if (!_tzi.dst)
    {
      _dstsw[0].time = TIME64_T_MIN;
      _dstsw[0].shift = 0;
      _dstsw[1].time = TIME64_T_MAX;
      _dstsw[1].shift = 0;
      _dstsw_count = 2;
      return;
    }

  i = 0;
  for (y = 0; _year_day[y] != SHRT_MAX; ++y)
    {
      month_table = (_leap_year (y + 1900)
                     ? _month_day_leap : _month_day_non_leap);
      d_year = _year_day[y];
      ywday = d_year + 4;       /* 01-Jan-1970 was a Thursday, ie, 4 */
      t_year = (time64_t)d_year * 24 * 60 * 60;

      d_start = switch_day (_tzi.sm, _tzi.sw, _tzi.sd, ywday, month_table);
      t_start = (time64_t)d_start * 24 * 60 * 60 + _tzi.st + _tzi.tz;
      if (ADD_OK (t_start, t_year))
        t_start += t_year;
      else
        t_start = t_year >= 0 ? TIME64_T_MAX : TIME64_T_MIN;

      d_end = switch_day (_tzi.em, _tzi.ew, _tzi.ed, ywday, month_table);
      t_end = (time64_t)d_end * 24 * 60 * 60 + _tzi.et + _tzi.tz - _tzi.shift;
      if (ADD_OK (t_end, t_year))
        t_end += t_year;
      else
        t_end = t_year >= 0 ? TIME64_T_MAX : TIME64_T_MIN;

      if (d_start < d_end || (d_start == d_end && _tzi.st <= _tzi.et))
        {
          /* Northern hemisphere. */

          if (i == 0)
            {
              _dstsw[0].time = TIME64_T_MIN;
              _dstsw[0].shift = 0;
              ++i;
            }
          _dstsw[i].time = t_start;
          _dstsw[i].shift = _tzi.shift;
          ++i;
          _dstsw[i].time = t_end;
          _dstsw[i].shift = 0;
          ++i;
        }
      else
        {
          /* Southern hemisphere. */

          if (i == 0)
            {
              _dstsw[0].time = TIME64_T_MIN;
              _dstsw[0].shift = _tzi.shift;
              ++i;
            }
          _dstsw[i].time = t_end;
          _dstsw[i].shift = 0;
          ++i;
          _dstsw[i].time = t_start;
          _dstsw[i].shift = _tzi.shift;
          ++i;
        }
    }
  _dstsw[i].time = TIME64_T_MAX;
  _dstsw[i].shift = TIME_T_MAX;
  ++i;
  _dstsw_count = i;

  assert (_dstsw_count == sizeof (_dstsw) / sizeof (_dstsw[0]));
}


static struct _dstswitch *find_switch (time64_t t)
{
  int lo, hi, i;

  if (t == TIME64_T_MAX)
    {
      i = _dstsw_count - 2;
      assert (_dstsw[i].time <= t);
      assert (t <= _dstsw[i+1].time);
    }
  else
    {
      lo = 0; hi = _dstsw_count - 2;
      for (;;)
        {
          i = (lo + hi) / 2;
          if (_dstsw[i].time > t)
            hi = i - 1;
          else if (_dstsw[i+1].time <= t)
            lo = i + 1;
          else
            break;
        }
      assert (_dstsw[i].time <= t);
      assert (t < _dstsw[i+1].time);
    }
  return &_dstsw[i];
}


/* Note that this function returns -1 on error.  Previously, it
   returned -1 if it could not determine whether daylight saving
   applies or not. */

int _gmt2loc (time_t *p)
{
  struct _dstswitch *sw;

  sw = find_switch (*p);
  if (!ADD_OK (*p, _tzi.tz - sw->shift))
    return -1;
  *p -= _tzi.tz - sw->shift;
  return sw->shift != 0;
}

/**
 * Convert from gmt to local time.
 *
 * @returns 1 if dts.
 * @returns 0 if not dts.
 * @returns -1 on over/under flow.
 * @param   p   The time to convert.
 */
int _gmt2loc64 (time64_t *p)
{
    struct _dstswitch *sw;
    sw = find_switch (*p);
    time64_t t = *p - (_tzi.tz - sw->shift);
    if (_tzi.tz - sw->shift > 0 ? t > *p : t < *p)
        return -1;
    *p = t;
    return sw->shift != 0;
}


/* Note that this function returns -1 on error (overflow).
   Previously, it returned -1 if it could not determine whether
   daylight saving applies or not. */

int _loc2gmt (time_t *p, int is_dst)
{
  time_t x;
  struct _dstswitch *sw;
  int count;

  if (is_dst > 0)
    {
      /* Our caller says that *P is specified as DST.  Compute UTC
         from *P, assuming that daylight saving applies. */

      if (!ADD_OK (*p, _tzi.tz - _tzi.shift))
        return -1;
      x = *p + _tzi.tz - _tzi.shift;
      sw = find_switch (x);
      *p = x;
      return sw->shift != 0;
    }
  else if (is_dst == 0)
    {
      /* Our caller says that *P is specified as standard time.
         Compute UTC from *P, assuming that daylight saving does not
         apply. */

      if (!ADD_OK (*p, _tzi.tz))
        return -1;
      x = *p + _tzi.tz;
      sw = find_switch (x);
      *p = x;
      return sw->shift != 0;
    }
  else
    {
      /* Our caller does not know whether *P is specified as DST or
         standard time.  Try to find out.  First try DST. */

      count = 0;
      if (ADD_OK (*p, _tzi.tz - _tzi.shift))
        {
          x = *p + _tzi.tz - _tzi.shift;
          sw = find_switch (x);
          if (sw->shift != 0)
            {
              *p = x;
              return 1;         /* DST */
            }
          ++count;
        }

      if (ADD_OK (*p, _tzi.tz))
        {
          x = *p + _tzi.tz;
          sw = find_switch (x);
          if (sw->shift == 0)
            {
              *p = x;
              return 0;         /* Not DST */
            }
          ++count;
        }

      if (count != 2)
        return -1;              /* Overflow */

      /* Assume that DST does not apply in the gap.  This makes moving
         into the gap from below work, but breaks moving into the gap
         from above.  The advantage of this choice is that ftime()
         works correctly in the gap if the clock is not adjusted. */

      *p += _tzi.tz;
      return 0;                 /* Not DST */
    }
}


/**
 * Convert from local time to gmt.
 *
 * @returns 1 if dts.
 * @returns 0 if not dts.
 * @returns -1 on over/under flow.
 * @param   p   The time to convert.
 */
int _loc2gmt64(time64_t *p, int is_dst)
{
    time64_t x;
    struct _dstswitch *sw;
    int count;

    if (is_dst > 0)
    {
        /* Our caller says that *P is specified as DST.  Compute UTC
           from *P, assuming that daylight saving applies. */

        x = *p + (_tzi.tz - _tzi.shift);
        if (_tzi.tz - _tzi.shift > 0 ? x < *p : x > *p)
            return -1;
        sw = find_switch (x);
        *p = x;
        return sw->shift != 0;
    }
    else if (is_dst == 0)
    {
        /* Our caller says that *P is specified as standard time.
           Compute UTC from *P, assuming that daylight saving does not
           apply. */

        x = *p + _tzi.tz;
        if (_tzi.tz > 0 ? x < *p : x > *p)
            return -1;
        sw = find_switch (x);
        *p = x;
        return sw->shift != 0;
    }
    else
    {
        /* Our caller does not know whether *P is specified as DST or
           standard time.  Try to find out.  First try DST. */

        count = 0;
        x = *p + _tzi.tz - _tzi.shift;
        if (!(_tzi.tz - _tzi.shift > 0 ? x < *p : x > *p))
        {
            sw = find_switch (x);
            if (sw->shift != 0)
            {
                *p = x;
                return 1;         /* DST */
            }
            ++count;
        }

        x = *p + _tzi.tz;
        if (!(_tzi.tz > 0 ? x < *p : x > *p))
        {
            sw = find_switch (x);
            if (sw->shift == 0)
            {
                *p = x;
                return 0;         /* Not DST */
            }
            ++count;
        }

        if (count != 2)
            return -1;              /* Overflow */

        /* Assume that DST does not apply in the gap.  This makes moving
           into the gap from below work, but breaks moving into the gap
           from above.  The advantage of this choice is that ftime()
           works correctly in the gap if the clock is not adjusted. */

        *p += _tzi.tz;
        return 0;                 /* Not DST */
    }
}
