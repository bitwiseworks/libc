/* bishlw.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <emx/bigint.h>

int _bi_shl_w (_bi_bigint *dst, int dst_words, _bi_word src, int shift)
{
  int b, w, dn;
  _bi_dword tmp;
  _bi_word carry;

  if (src == 0)
    {
      dst->n = 0;
      return 0;
    }
  b = (unsigned)shift % _BI_WORDSIZE; /* Bit shift count */
  w = (unsigned)shift / _BI_WORDSIZE; /* Word shift count */
  if (w + 1 > dst_words)
    return 1;
  for (dn = 0; dn < w; ++dn)
    dst->v[dn] = 0;
  tmp = (_bi_dword)src << b;
  dst->v[dn++] = (_bi_word)tmp;
  carry = tmp >> _BI_WORDSIZE;
  if (carry)
    {
      if (dn + 1 > dst_words)
        return 1;
      dst->v[dn++] = carry;
    }
  dst->n = dn;
  return 0;
}
