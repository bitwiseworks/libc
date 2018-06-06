/* bimulbw.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include <emx/bigint.h>

/* DST==SRC is allowed. */

int _bi_mul_bw (_bi_bigint *dst, int dst_words,
                const _bi_bigint *src, _bi_word factor)
{
  int i;
  _bi_dword tmp;
  _bi_word carry;

  if (factor == 0)
    {
      dst->n = 0;
      return 0;
    }
  carry = 0;
  for (i = 0; i < src->n; ++i)
    {
      tmp = (_bi_dword)src->v[i] * factor + carry;
      dst->v[i] = (_bi_word)tmp;
      carry = tmp >> _BI_WORDSIZE;
    }
  dst->n = src->n;
  if (carry != 0)
    {
      if (dst->n >= dst_words)
        return 1;
      dst->v[dst->n++] = carry;
    }
  return 0;
}
