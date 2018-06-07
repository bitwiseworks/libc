/* biaddbb.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include <emx/bigint.h>

/* DST==SRC1 or DST==SRC2 is allowed. */

int _bi_add_bb (_bi_bigint *dst, int dst_words,
                const _bi_bigint *src1, const _bi_bigint *src2)
{
  int i, n;
  _bi_dword tmp;
  _bi_word carry;
  const _bi_bigint *s;

  if (src1->n < src2->n)
    {
      n = src1->n; s = src2;
    }
  else
    {
      n = src2->n; s = src1;
    }
  if (s->n > dst_words)
    return 1;
  carry = 0;
  for (i = 0; i < n; ++i)
    {
      tmp = (_bi_dword)src1->v[i] + src2->v[i] + carry;
      dst->v[i] = (_bi_word)tmp;
      carry = tmp >> _BI_WORDSIZE;
    }
  for (; i < s->n; ++i)
    {
      tmp = (_bi_dword)s->v[i] + carry;
      dst->v[i] = (_bi_word)tmp;
      carry = tmp >> _BI_WORDSIZE;
    }
  if (carry)
    {
      if (i + 1 > dst_words)
        return 1;               /* TODO: restore DST? */
      dst->v[i++] = carry;
    }
  dst->n = i;
  return 0;
}
