/* bicmpp2.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <emx/bigint.h>

/* SHIFT2 may be negative. */

int _bi_cmp_pow2 (const _bi_bigint *src1, int shift2)
{
  int shift1, i, words, bits;

  if (shift2 < 0)
    {
      /* 0 < src2 < 1 */
      return src1->n == 0 ? -1 : 1;
    }

  shift1 = _bi_fls (src1) - 1;
  if (shift1 < shift2)
    return -1;
  if (shift1 > shift2)
    return 1;

  words = (unsigned)shift2 / _BI_WORDSIZE;
  bits = (unsigned)shift2 % _BI_WORDSIZE;

  for (i = 0; i < words; ++i)
    if (src1->v[i] != 0)
      return 1;
  if (bits == 0)
    return 0;
  if (src1->v[words] & (((_bi_word)1 << bits) - 1))
    return 1;
  return 0;
}
