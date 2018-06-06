/* atod.c (emx+gcc) -- Copyright (c) 1996-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <locale.h>
#include <errno.h>
#include <assert.h>
#include <emx/bigint.h>
#include <emx/float.h>
#include <InnoTekLIBC/locale.h>
#include <ctype.h>


#define BBITS           (LDBL_MAX_EXP + LDBL_MANT_DIG - LDBL_MAX_10_EXP \
                         + (DECIMAL_DIG * 10)/3)
#define BWORDS          (_BI_WORDS (BBITS) + 1)


#define MAX1    (_BI_WORDMAX / 10)
#define MAX2    (_BI_WORDMAX - 10 * MAX1)

#define DOT     (unsigned char)__libc_gLocaleLconv.s.decimal_point[0]

const char * __legacy_atod (long double *p_result, const char *string,
                     int min_exp, int max_exp, int bias,
                     int mant_dig, int decimal_dig,
                     int max_10_exp, int min_den_10_exp)
{
  const char *digits, *digits_end, *s, *end;
  int e, k, i, cmp, int_digits, frac_digits, ov, pt, next, neg;
  unsigned long long mant;
  _bi_word w1, w2;
  _BI_DECLARE (f, BWORDS);
  _BI_DECLARE (t, BWORDS);      /* r, temporary */
  _BI_DECLARE (x, BWORDS);      /* x and q */
  _BI_DECLARE (uv, BWORDS);     /* u or v */
  union
  {
    long double ld;
    struct
    {
      unsigned long long mant;
      unsigned short exp;
    } x;
  } z;

#define INTEGER_BIT     (1ULL << (mant_dig - 1))

  _BI_INIT (f);
  _BI_INIT (t);
  _BI_INIT (x);
  _BI_INIT (uv);

  s = string;
  while (*s != 0 && isspace ((unsigned char)*s))
    ++s;
  neg = 0;
  if (*s == '-')
    {
      ++s; neg = 1;
    }
  else if (*s == '+')
    ++s;

  if ((s[0] == 'i' || s[0] == 'I')
      && (s[1] == 'n' || s[1] == 'N')
      && (s[2] == 'f' || s[2] == 'F'))
    {
      s += 3;
      if ((s[0] == 'i' || s[0] == 'I')
          && (s[1] == 'n' || s[1] == 'N')
          && (s[2] == 'i' || s[2] == 'I')
          && (s[3] == 't' || s[3] == 'T')
          && (s[4] == 'y' || s[4] == 'Y'))
        s += 5;
      *p_result = neg ? -INFINITY : INFINITY;
      return s;
    }
  else if ((s[0] == 'n' || s[0] == 'N')
           && (s[1] == 'a' || s[1] == 'A')
           && (s[2] == 'n' || s[2] == 'N'))
    {
      s += 3;
      end = s;
      if (end[0] == '(')
        {
          ++end;
          while (*end == '_' || (*end >= '0' && *end <= '9')
                 || (*end >= 'a' && *end <= 'z')
                 || (*end >= 'A' && *end <= 'Z'))
            ++end;
          if (*end == ')')
            ++end;
          else
            end = s;
        }
      *p_result = NAN;
      return end;
    }

  digits = s;
  pt = 0; int_digits = frac_digits = 0;
  while ((*s >= '0' && *s <= '9') || (*s == DOT && !pt))
    {
      if (*s++ == DOT)
        pt = 1;
      else if (pt)
        ++frac_digits;
      else
        ++int_digits;
    }
  digits_end = s;

  if (int_digits == 0 && frac_digits == 0)
    {
      /* Subject sequence is empty or does not have the expected
         form. */

      *p_result = 0.0;
      return string;
    }

  /* Read exponent. */

  e = 0; end = s;
  if (*s == 'e' || *s == 'E')
    {
      char *exp_end, exp_neg;
      long exp;
      int saved_errno;

      ++s; exp_neg = 0;
      if (*s == '+')
        ++s;
      else if (*s == '-')
        {
          exp_neg = 1; ++s;
        }
      if (isdigit ((unsigned char)*s)) /* Note: strtol() skips whitespace! */
        {
          saved_errno = errno; errno = 0;
          exp = strtol (s, &exp_end, 10);
          if (errno == ERANGE)
            {
              /* If the exponent overflows, we need a *very* long
                 mantissa to get a valid number.  Such a mantissa
                 doesn't fit into memory. */

              end = exp_end;
              if (exp_neg)
                goto underflow;
              else
                goto overflow;
            }
          else if (errno == 0 && exp_end != s)
            {
              end = exp_end;
              e = exp_neg ? -exp : exp;
            }
          errno = saved_errno;
        }
    }

  /* Adjust the exponent so that we can treat the sequence of digits
     as integer (by ignoring the decimal point). */

  e -= frac_digits;

  /* Remove leading zeros to keep numbers small. */

  while (digits < digits_end && (*digits == '0' || *digits == DOT))
    ++digits;

  /* Remove trailing zeros to keep numbers small. */

  while (digits_end > digits
         && (digits_end[-1] == '0' || digits_end[-1] == DOT))
    {
      if (digits_end[-1] != DOT)
        ++e;
      --digits_end;
    }

  /* Return 0.0 if the number is zero. */

  if (digits == digits_end)
    {
      *p_result = neg ? -0.0 : 0.0;
      return end;
    }

  /* Quick test of the exponent.  Note that the string may or may not
     contain a decimal point. */

  if (e + digits_end - digits - 2 > max_10_exp)
    goto overflow;
  if (e + digits_end - digits < min_den_10_exp)
    goto underflow;

  /* Build the integer. */

  f.n = 0;                      /* f := 0 */
  w1 = 0;                       /* Accumulates up to 9 digits */
  w2 = 1;                       /* 10 ^ number_of_digits */
  k = 0;                        /* Truncate after DECIMAL_DIG digits */
  for (s = digits; s < digits_end; ++s)
    if (*s != DOT)
      {
        if (k == decimal_dig)
          ++e;
        else
          {
            /* We have to check both W1 and W2 for overflow. */

            if (w2 >= MAX1 || (w1 >= MAX1 && (w1 > MAX1 || *s - '0' > MAX2)))
              {
                /* Overflow of the accumulator.  Append the digits
                   from W1 to F. */

                ov = _bi_mul_bw (&f, BWORDS, &f, w2);
                assert (!ov);
                if (w1 != 0)
                  {
                    t.n = 1; t.v[0] = w1;
                    ov = _bi_add_bb (&f, BWORDS, &f, &t);
                    assert (!ov);
                  }
                w1 = 0; w2 = 1;
              }
            w1 = w1 * 10 + *s - '0';
            w2 = w2 * 10;
            ++k;
          }
      }

  /* Check the exponent again, based on k -- now we don't have to cope
     with the decimal point and get a better result. */

  if (e + k - 1 > max_10_exp)
    goto overflow;
  if (e + k < min_den_10_exp)
    goto underflow;

  /* If there are digits accumulated in W1, append them to F. */

  if (w2 != 1)
    {
      ov = _bi_mul_bw (&f, BWORDS, &f, w2);
      assert (!ov);
      t.n = 1; t.v[0] = w1;
      ov = _bi_add_bb (&f, BWORDS, &f, &t);
      assert (!ov);
    }

  /* Now, the number to convert is F*10^E.  The following code is
     based on AlgorithmM of [1]. */

  k = bias + mant_dig - 1;

  /* We use powers of 5, not 10, to make the numbers smaller.  Adjust
     K appropriately. */

  k += e;

  if (e < 0)
    {
      ov = _bi_pow5 (&uv, BWORDS, -e, 0, NULL);
      assert (!ov);

      /* Estimate the scaling factor. */

      i = _bi_fls (&f) - _bi_fls (&uv);
      if (k + i - mant_dig < 1)
        i = mant_dig - k + 1;
      k += i - mant_dig;
      if (i < mant_dig)
        {
          ov = _bi_shl_b (&f, BWORDS, &f, mant_dig - i);
          assert (!ov);
        }
      else if (i > mant_dig)
        {
          ov = _bi_shl_b (&uv, BWORDS, &uv,  i - mant_dig);
          assert (!ov);
        }

      _bi_set_b (&t, BWORDS, &f);
      ov = _bi_div_rem_bb (&t, BWORDS, &x, BWORDS, &uv); /* x:=r/v, r:=r%v */
      assert (!ov);
      i = _bi_fls (&x);

      /* Our estimate for K is never too big, except for denormals. */

      if (i < mant_dig)
        {
          --k;
          assert (k == 0);
        }

      /* Sometimes, our estimate for K is too small. */

      else if (i > mant_dig)
        {
          ++k;
          if (k > max_exp - min_exp + 1)
            goto overflow;
          ov = _bi_shl_b (&uv, BWORDS, &uv, 1);
          assert (!ov);
          _bi_set_b (&t, BWORDS, &f);

          /* We have to compute the quotient and remainder again. */

          ov = _bi_div_rem_bb (&t, BWORDS, &x, BWORDS, &uv);
          assert (!ov);

          /* Our estimate can never be off by more than one, according
             to Theorem 1. */

          assert (_bi_fls (&x) == mant_dig);
        }

      /* r < v - r  is equivalent to 2r < v. */

      ov = _bi_shl_b (&t, BWORDS, &t, 1);
      assert (!ov);
      cmp = _bi_cmp_bb (&t, &uv);
    }
  else
    {
      ov = _bi_pow5 (&t, BWORDS, e, 0, NULL);
      assert (!ov);
      ov = _bi_mul_bb (&uv, BWORDS, &f, &t);
      assert (!ov);

      /* v is a power of two, computing the scaling factor is
         trivial. */

      i = _bi_fls (&uv);
      k += i - mant_dig;
      if (i < mant_dig)
        {
          ov = _bi_shl_b (&uv, BWORDS, &uv, mant_dig - i);
          assert (!ov);
          i = 0;
        }
      else if (i > mant_dig)
        i = i - mant_dig;
      else
        i = 0;

      /* Compute x:=u/v and u:=u MOD v where v=2^i. */

      ov = _bi_div_rem_pow2 (&uv, &x, BWORDS, i);
      assert (!ov);

      assert (x.n == 2);

      /* r < v - r is equivalent to 2r < v.  Note that we have u=r and
         v=2^i here. */

      cmp = _bi_cmp_pow2 (&uv, i - 1);
    }

  if (k > max_exp - min_exp + 1)
    goto overflow;

  assert (x.n <= 2);            /* x.n can be < 2 for denormals */
  if (x.n == 0)
    mant = 0;
  else if (x.n == 1)
    mant = x.v[0];
  else
    mant = ((unsigned long long)x.v[1] << _BI_WORDSIZE) | x.v[0];

  if (cmp < 0)
    next = 0;
  else if (cmp > 0)
    next = 1;
  else
    next = x.v[0] & 1;
  if (next)
    {
      if ((mant & ~INTEGER_BIT) == INTEGER_BIT - 1)
        {
          mant = INTEGER_BIT;
          ++k;
          if (k > max_exp - min_exp + 1)
            goto overflow;
        }
      else
        ++mant;
    }

  if (mant == 0)
    goto underflow;

  /* Build the floating point number from the sign, exponent, and
     significant. */

  switch (mant_dig)
    {
    case 24:
      /* Clear the integer bit. */
      mant &= ~(1ULL << (FLT_MANT_DIG - 1));
      /* Insert the exponent. */
      mant |= ((unsigned long long)k << (FLT_MANT_DIG - 1));
      if (neg)
        mant |= 0x80000000ULL;
      *p_result = *(float *)(void *)&mant;
      break;

    case 53:
      /* Clear the integer bit. */
      mant &= ~(1ULL << (DBL_MANT_DIG - 1));
      /* Insert the exponent. */
      mant |= ((unsigned long long)k << (DBL_MANT_DIG - 1));
      if (neg)
        mant |= 0x8000000000000000ULL;
      *p_result = *(double *)(void *)&mant;
      break;

    case 64:
      z.x.mant = mant;
      z.x.exp = k;
      if (neg)
        z.x.exp |= 0x8000;
      *p_result = z.ld;
      break;
    }
  return end;

overflow:
  *p_result = neg ? -HUGE_VALL : HUGE_VALL;
  errno = ERANGE;
  return end;

underflow:
  *p_result = neg ? -0.0 : 0.0;
  errno = ERANGE;
  return end;
}


/* Theorem 1.  If u is a normalized m+n-bit number and v is a
   normalized n-bit number, then 2^(m-1) <= [u/v] < 2^(m+1).

   Proof: To get a lower bound for [u/v] consider the worst case
   of u=1000... (u=2^(m+n-1)) and v=1111... (v=2^n - 1).

     [u/v] >= [2^(m+n-1) / (2^n - 1)]
           >= [2^(m+n-1) / 2^n]
            = 2^(m-1)

   To get an upper bound for [u/v] consider the worst case of
   u=1111... (u=2^(m+n)-1) and v=1000... (v=2^(n-1)).

     [u/v] <= [(2^(m+n) - 1) / 2^(n-1)]
            = [2^(m+1) - 2^(1-n)]
            < [2^(m+1)]
            = 2^(m+1)                           qed. */

/* References:

   [1] William D. Clinger,
       How to read floating point numbers accurately,
       ACM SIGPLAN notices Volume 25 Number 6, June 1990. */
