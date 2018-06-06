/* dtoa.c (emx+gcc) -- Copyright (c) 1996-1999 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <sys/builtin.h>
#include <sys/param.h>
#include <emx/float.h>
#include <emx/bigint.h>

#define BWORDS          _BI_WORDS (LDBL_MAX_EXP)
#define BIAS            16383

struct long_double_bits
{
  _bi_dword mant;
  unsigned short exp : 15;
  unsigned short sign : 1;
};


static int estimate (int e)
{
  if (e < 0)
    return (int)(e*0.3010299956639812);
  else
    return 1 + (int)(e*0.3010299956639811);
}


char *__legacy_dtoa (char *buffer, int *p_exp,
                     long double x, int ndigits, int fmt, int dig)
{
  const struct long_double_bits *bits;
  _bi_dword f;
  int e, k, r_n, s_n, m_n, i, cmp, digit;
  int di, s_pow2, a, ov;
  _BI_DECLARE (r, BWORDS);
  _BI_DECLARE (s, BWORDS);
  _BI_DECLARE (m, BWORDS);
  _BI_DECLARE (tmp, BWORDS);

  _BI_INIT (r); _BI_INIT (s); _BI_INIT (m); _BI_INIT (tmp);

  bits = (const struct long_double_bits *)(void *)&x;
  f = bits->mant;
  e = bits->exp;

  /* E is the exponent for the fraction F; now replace E with the
     exponent for the integer F.  Compute an estimate for the scaling
     factor.  See [1]. */

  if (e != 0)                   /* normal */
    {
      e -= BIAS + LDBL_MANT_DIG - 1;
      k = estimate (e + LDBL_MANT_DIG - 1);
    }
  else                          /* denormal */
    {
      e = 1 - (BIAS + LDBL_MANT_DIG - 1);
      if (f >> 32 == 0)
        k = estimate (e + __fls ((unsigned long)f) - 1);
      else
        k = estimate (e + __fls ((unsigned long)(f >> 32)) + 32 - 1);
    }

  /* Compute initial values (cf. table 1 of [1]). */

  if (e >= 0)
    {
      if (f != 1ULL << (LDBL_MANT_DIG-1))
        {
          r_n = e + 1; s_n = 1; m_n = e;
        }
      else
        {
          r_n = e + 2; s_n = 2; m_n = e + 1;
        }
    }
  else
    {
      if (e == LDBL_MIN_EXP - LDBL_MANT_DIG || f != 1ULL << (LDBL_MANT_DIG-1))
        {
          r_n = 1; s_n = 1 - e; m_n = 0;
        }
      else
        {
          r_n = 2; s_n = 2 - e; m_n = 1;
        }
    }

  /* Now compute R, S, and M from K and the shift counts. */

  if (k == 0)
    {
      ov = _bi_set_d (&tmp, BWORDS, f); assert (!ov);
      ov = _bi_shl_b (&r, BWORDS, &tmp, r_n); assert (!ov);
      ov = _bi_shl_w (&s, BWORDS, 1, s_n); assert (!ov);
      s_pow2 = 1;
      ov = _bi_shl_w (&m, BWORDS, 1, m_n); assert (!ov);
    }
  else if (k > 0)
    {
      s_n += k;                 /* 10^k = 2^k * 5^k */

      /* Reduce powers of two.  Note that M is the smallest one of M,
         R, and S.  This speed hack has been borrowed from Robert
         G. Burger's C code. */

      i = MIN (m_n, s_n);
      r_n -= i; s_n -= i; m_n -= i;

      ov = _bi_set_d (&tmp, BWORDS, f); assert (!ov);
      ov = _bi_shl_b (&r, BWORDS, &tmp, r_n); assert (!ov);
      ov = _bi_pow5 (&s, BWORDS, k, s_n, 0); assert (!ov);
      s_pow2 = 0;
      ov = _bi_shl_w (&m, BWORDS, 1, m_n); assert (!ov);
    }
  else                          /* k < 0 */
    {
      s_n += k;                 /* 10^k = 2^k * 5^k */

      ov = _bi_set_d (&tmp, BWORDS, f); assert (!ov);
      ov = _bi_pow5 (&r, BWORDS, -k, r_n, &tmp); assert (!ov);
      ov = _bi_shl_w (&s, BWORDS, 1, s_n); assert (!ov);
      s_pow2 = 1;
      ov = _bi_pow5 (&m, BWORDS, -k, m_n, 0); assert (!ov);
    }

  /* Adjust the estimate, that is, multiply M and R by 10 if the
     estimate is correct (it's frequently incorrect): Multiply M and R
     by 10 (and decrement K) if R + M <= S. */

  ov = _bi_add_bb (&tmp, BWORDS, &r, &m); assert (!ov);
  cmp = _bi_cmp_bb (&tmp, &s);
  if (cmp <= 0)
    {
      k -= 1;
      ov = _bi_mul_bw (&r, BWORDS, &r, 10); assert (!ov);
    }

  /* Compute the maximum number of digits to generate. */

  switch (fmt)
    {
    case DTOA_PRINTF_E:
      a = ndigits + 1;
      break;
    case DTOA_PRINTF_F:
      a = k + 1 + ndigits;
      break;
    case DTOA_PRINTF_G:
    case DTOA_GCVT:
      a = ndigits;
      break;

    default:
      abort ();
    }
  if (a > DECIMAL_DIG)
    a = DECIMAL_DIG;

  /* Generate the digits.  We might need to use buffer[0] in case of
     carry produced by rounding.  (Unlike the algorithms of [1] and
     [2], this one does _not_ generate the digits left to right.) */

  buffer[0] = '0'; di = 1;

  if (a > 0)
    {
      for (;;)
        {
          /* Speed hack borrowed from Robert G. Burger's C code:
             dividing by powers of two is faster than dividing by
             arbitrary numbers.  */

          if (s_pow2)
            digit = _bi_wdiv_rem_pow2 (&r, s_n);
          else
            digit = _bi_hdiv_rem_b (&r, &s);
          assert (digit < 10);

          buffer[di++] = digit + '0';

          if (--a <= 0)
            break;

          ov = _bi_mul_bw (&r, BWORDS, &r, 10); assert (!ov);
        }

      /* We've generated all the digits but might have to round up. */

      ov = _bi_shl_b (&tmp, BWORDS, &r, 1); assert (!ov);
      cmp = _bi_cmp_bb (&tmp, &s);
      if (cmp > 0 || (cmp == 0 && ((buffer[di-1] - '0') & 1)))
        while (++buffer[di-1] > '9')
          --di;
    }
  else if (a == 0)
    {
      /* See above. */

      if (s_pow2)
        digit = _bi_wdiv_rem_pow2 (&r, s_n);
      else
        digit = _bi_hdiv_rem_b (&r, &s);
      assert (digit < 10);

      /* Round to nearest or even (note: zero is even, so we don't
         round up if S/R is exactly five). */

      if (digit > 5 || (digit == 5 && r.n != 0))
        buffer[0] = '1';
    }

  while (di < DECIMAL_DIG + 1)  /* One extra for buffer[0] */
    buffer[di++] = '0';
  buffer[di] = 0;

  if (buffer[0] != '0')
    {
      *p_exp = k + 1;
      return buffer;
    }
  else
    {
      *p_exp = k;
      return buffer + 1;
    }
}

/* References:

   [1] Robert G. Burger and R. Kent Dybvig,
       Printing floating-point numbers quickly and accurately.
       ACM SIGPLAN notices Volume 31 Number 5, May 1996.

   [2] Guy L. Steele Jr. and Jon L. White,
       How to print floating-point numbers accurately.
       ACM SIGPLAN notices Volume 25 Number 6, June 1990.

   (Not yet used:)

   [3] David M. Gay,
       Correctly rounded binary-decimal and decimal-binary conversions.
       Numerical Analysis Manuscript 90-10, AT&T Bell Laboratories,
       Murray Hill, New Jersey 07974, November 1990. */
