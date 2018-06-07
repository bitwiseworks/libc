/* biwdivp2.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <assert.h>
#include <emx/bigint.h>

/* Return NUM/(1<<SHIFT), replace NUM with NUM%(1<<SHIFT).  The
   quotient must fit in a _bi_word. */

_bi_word _bi_wdiv_rem_pow2 (_bi_bigint *num, int shift)
{
  int shift_words, shift_bits, n;
  _bi_word q;

  assert (shift >= 1);

  shift_words = (unsigned)shift / _BI_WORDSIZE + 1;
  n = num->n;

  /* The quotient is zero if NUM is zero or if NUM < (1<<SHIFT). */

  if (n < shift_words)
    return 0;

  assert (n <= shift_words + 1); /* Overflow must not occur */
  assert (num->v[n-1] != 0); /* NUM must be normalized */

  shift_bits = (unsigned)shift % _BI_WORDSIZE;

  if (n == shift_words)
    q = num->v[n-1] >> shift_bits;
  else /* n == shift_words + 1 */
    {
      assert (n >= 2);
      assert (shift_bits != 0);
      q = (((num->v[n-1] << (_BI_WORDSIZE - shift_bits))
            | (num->v[n-2]) >> shift_bits));
      n -= 1;                   /* Drop the most significant word */
    }
  num->v[n-1] &= ((_bi_word)1 << shift_bits) - 1;
  while (n != 0 && num->v[n-1] == 0)
    --n;
  num->n = n;
  return q;
}
