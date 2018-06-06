/* bicmpbb.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <emx/bigint.h>

int _bi_cmp_bb (const _bi_bigint *src1, const _bi_bigint *src2)
{
  int i;

  if (src1->n > src2->n)
    return 1;
  else if (src1->n < src2->n)
    return -1;
  for (i = src1->n - 1; i >= 0; --i)
    if (src1->v[i] > src2->v[i])
      return 1;
    else if (src1->v[i] < src2->v[i])
      return -1;
  return 0;
}
