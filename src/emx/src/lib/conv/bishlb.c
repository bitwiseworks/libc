/* bishlb.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <emx/bigint.h>

/* DST==SRC is allowed. */

int _bi_shl_b (_bi_bigint *dst, int dst_words,
               const _bi_bigint *src, int shift)
{
  int b, w, i, sn, dn;
  _bi_word tmp;

  sn = src->n;
  if (sn == 0)
    {
      dst->n = 0;
      return 0;
    }
  b = (unsigned)shift % _BI_WORDSIZE; /* Bit shift count */
  w = (unsigned)shift / _BI_WORDSIZE; /* Word shift count */
  dn = sn + w;
  if (dn > dst_words)
    return 1;
  if (b == 0)
    for (i = sn-1; i >= 0; --i)
      dst->v[i+w] = src->v[i];
  else
    {
      tmp = src->v[sn-1] >> (_BI_WORDSIZE - b);
      if (tmp != 0)
        {
          if (dn + 1 > dst_words)
            return 1;
          dst->v[dn++] = tmp;
        }
      for (i = sn-1; i > 0; --i)
        {
          tmp = (src->v[i] << b) | (src->v[i-1] >> (_BI_WORDSIZE - b));
          dst->v[i+w] = tmp;
        }
      dst->v[w] = src->v[0] << b;
    }
  for (i = 0; i < w; ++i)
    dst->v[i] = 0;
  dst->n = dn;
  return 0;
}
