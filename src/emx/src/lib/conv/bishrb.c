/* bishrb.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <emx/bigint.h>

/* DST==SRC is allowed. */

int _bi_shr_b (_bi_bigint *dst, int dst_words, const _bi_bigint *src,
               int shift)
{
  int b, w, i, sn, dn;
  _bi_word tmp;

  b = (unsigned)shift % _BI_WORDSIZE; /* Bit shift count */
  w = (unsigned)shift / _BI_WORDSIZE; /* Word shift count */
  sn = src->n;
  if (sn <= w)
    {
      dst->n = 0;
      return 0;
    }
  dn = sn - w;
  if (dn > dst_words)
    return 1;
  if (b == 0)
    for (i = 0; i < sn; ++i)
      dst->v[i] = src->v[i+w];
  else
    {
      for (i = 0; i < dn - 1; ++i)
        dst->v[i] = (src->v[i+w] >> b) | (src->v[i+w+1] << (_BI_WORDSIZE - b));
      tmp = src->v[dn-1+w] >> b;
      if (tmp != 0)
        dst->v[dn-1] = tmp;
      else
        dn -= 1;
    }
  dst->n = dn;
  return 0;
}
