/* bisetw.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <emx/bigint.h>

int _bi_set_w (_bi_bigint *dst, int dst_words, _bi_word src)
{
  if (src == 0)
    dst->n = 0;
  else if (dst_words <= 0)
    return 1;
  else
    {
      dst->n = 1;
      dst->v[0] = src;
    }
  return 0;
}
