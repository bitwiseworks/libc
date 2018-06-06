/* bidivhlp.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include <stdlib.h>
#include <assert.h>
#include <emx/bigint.h>

_bi_word _bi_div_estimate (_bi_word *u, _bi_word v1, _bi_word v2)
{
  _bi_word q, r;

  assert (u[0] <= v1);
  if (u[0] == v1)
    {
      _bi_dword rd;
      q = (_bi_word)-1;
      rd = ((_bi_dword)u[0] << _BI_WORDSIZE) + u[-1] - (_bi_dword)q * v1;
      r = (_bi_word)rd;
      if (r != rd)
        return q;
    }
  else
    {
      _ulldiv_t qr;
      qr = _ulldiv (((_bi_dword)u[0] << _BI_WORDSIZE) + u[-1], v1);
      q = qr.quot;
      r = qr.rem;
    }

  /* Eliminate most cases where Q is one too large, eliminate all
     cases where Q is two too large. */

  while ((_bi_dword)v2 * q > ((_bi_dword)r << _BI_WORDSIZE) + u[-2])
    {
      assert (q != 0);
      q -= 1; r += v1;
      if (r < v1)               /* Overflow */
        break;
    }
  return q;
}


int _bi_div_subtract (_bi_word *u, const _bi_word *v, int n, _bi_word q)
{
  int i;
  _bi_word sub_borrow, mul_carry;
  _bi_sdword stmp;

  mul_carry = 0; sub_borrow = 0;
  for (i = 0; i < n; ++i)
    {
      stmp = (_bi_dword)*v++ * q + mul_carry;
      mul_carry = stmp >> _BI_WORDSIZE;
      stmp = (_bi_dword)*u - (_bi_word)stmp - sub_borrow;
      *u++ = (_bi_word)stmp;
      sub_borrow = stmp < 0;
    }
  stmp = (_bi_dword)*u - mul_carry - sub_borrow;
  *u = (_bi_word)stmp;
  return stmp < 0;
}


void _bi_div_add_back (_bi_word *u, const _bi_word *v, int n)
{
  int i;
  _bi_word carry;
  _bi_dword utmp;

  carry = 0;
  for (i = 0; i <= n; ++i)
    {
      utmp = (_bi_dword)*u + *v++ + carry;
      *u++ = (_bi_word)utmp;
      carry = utmp >> _BI_WORDSIZE;
    }
  assert (carry);
}
