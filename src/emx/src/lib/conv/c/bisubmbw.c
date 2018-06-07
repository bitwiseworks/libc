/* bisubmbw.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include <emx/bigint.h>

/* DST := DST - FACTOR * SRC; leave DST unchanged and return 1 if the
   result would be negative. */

int _bi_sub_mul_bw (_bi_bigint *dst, const _bi_bigint *src, _bi_word factor)
{
  int i, n;
  _bi_word sub_borrow, mul_carry, add_carry;
  _bi_sdword stmp;
  _bi_dword utmp;

  if (src->n > dst->n)
    return 1;

  if (src->n == 0 || factor == 0)
    return 0;

  n = src->n;
  sub_borrow = 0; mul_carry = 0;
  for (i = 0; i < n; ++i)
    {
      stmp = (_bi_dword)src->v[i] * factor + mul_carry;
      mul_carry = stmp >> _BI_WORDSIZE;
      stmp = (_bi_dword)dst->v[i] - (_bi_word)stmp - sub_borrow;
      dst->v[i] = (_bi_word)stmp;
      sub_borrow = stmp < 0;
    }
  n = dst->n;
  if (mul_carry && i < n)
    {
      stmp = (_bi_dword)dst->v[i] - mul_carry - sub_borrow;
      dst->v[i] = (_bi_word)stmp;
      mul_carry = 0;
      sub_borrow = stmp < 0;
      ++i;
    }
  while (sub_borrow && i < n)
    {
      stmp = (_bi_dword)dst->v[i] - sub_borrow;
      dst->v[i] = (_bi_word)stmp;
      sub_borrow = stmp < 0;
      ++i;
    }
  if (!mul_carry && !sub_borrow)
    {
      while (n != 0 && dst->v[n-1] == 0)
        --n;
      dst->n = n;
      return 0;
    }

  /* The result would be negative.  Restore the original DST. */

  add_carry = 0; mul_carry = 0;
  n = src->n;
  for (i = 0; i < n; ++i)
    {
      utmp = (_bi_dword)src->v[i] * factor + mul_carry;
      mul_carry = utmp >> _BI_WORDSIZE;
      utmp = (_bi_dword)dst->v[i] + (_bi_word)utmp + add_carry;
      dst->v[i] = (_bi_word)utmp;
      add_carry = utmp >> _BI_WORDSIZE;
    }
  n = dst->n;
  if (i < n && (mul_carry || add_carry))
    {
      utmp = (_bi_dword)dst->v[i] + mul_carry + add_carry;
      dst->v[i] = (_bi_word)utmp;
      mul_carry = 0;
      add_carry = utmp >> _BI_WORDSIZE;
      ++i;
    }
  return 1;
}
