/* bidivbw.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include <emx/bigint.h>

/* QUOT==NUM is allowed. */

int _bi_div_rem_bw (_bi_bigint *quot, int quot_words, _bi_word *rem,
                    const _bi_bigint *num, _bi_word den)
{
  int j;
  _bi_word r;
  _bi_dword t;

  if (den == 0)
    return 1;

  if (num->n > quot_words)
    return 1;

  if (num->n == 0)
    {
      *rem = 0;
      quot->n = 0;
      return 0;
    }

  /* TAOCP Vol. 2, 4.3.1, Exercise 16. */

  r = 0;
  for (j = num->n - 1; j >= 0; --j)
    {
      t = ((_bi_dword)r << _BI_WORDSIZE) + num->v[j];
      quot->v[j] = t / den;
      r = t % den;
    }
  *rem = r;
  if (quot->v[num->n-1] != 0)
    quot->n = num->n;
  else
    quot->n = num->n - 1;
  return 0;
}
