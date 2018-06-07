/* bisetd.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <emx/bigint.h>

int _bi_set_d (_bi_bigint *dst, int dst_words, _bi_dword src)
{
  if (src == 0)
    dst->n = 0;
  else if (dst_words <= 0)
    return 1;
  else if (src >> _BI_WORDSIZE == 0)
    {
      dst->n = 1;
      dst->v[0] = (_bi_word)src;
    }
  else if (dst_words <= 1)
    return 1;
  else
    {
      dst->n = 2;
      dst->v[0] = (_bi_word)src;
      dst->v[1] = (_bi_word)(src >> _BI_WORDSIZE);
    }
  return 0;
}
