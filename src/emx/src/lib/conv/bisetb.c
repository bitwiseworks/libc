/* bisetb.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>
#include <emx/bigint.h>

int _bi_set_b (_bi_bigint *dst, int dst_words, const _bi_bigint *src)
{
  if (src->n > dst_words)
    return 1;
  dst->n = src->n;
  memcpy (dst->v, src->v, src->n * sizeof (_bi_word));
  return 0;
}
