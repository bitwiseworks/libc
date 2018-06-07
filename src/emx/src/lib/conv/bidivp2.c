/* bidivp2.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <assert.h>
#include <emx/bigint.h>

int _bi_div_rem_pow2 (_bi_bigint *num_rem, _bi_bigint *quot, int quot_words,
                      int shift)
{
  int shift_words, shift_bits, n;

  assert (shift >= 0);
  if (shift == 0)
    {
      if (_bi_set_b (quot, quot_words, num_rem) != 0)
        return 1;
      num_rem->n = 0;
      return 0;
    }

  n = num_rem->n;
  shift_words = (unsigned)shift / _BI_WORDSIZE;
  shift_bits = (unsigned)shift % _BI_WORDSIZE;
  if (n <= shift_words)
    {
      quot->n = 0;
      return 0;
    }

  if (_bi_shr_b (quot, quot_words, num_rem, shift) != 0)
    return 1;

  n = shift_words + 1;
  num_rem->v[n-1] &= ((_bi_word)1 << shift_bits) - 1;
  while (n > 0 && num_rem->v[n-1] == 0)
    --n;
  num_rem->n = n;
  return 0;
}
