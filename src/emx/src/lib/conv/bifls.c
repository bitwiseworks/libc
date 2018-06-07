/* bifls.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>
#include <sys/builtin.h>
#include <emx/bigint.h>

int _bi_fls (const _bi_bigint *src)
{
  if (src->n == 0)
    return 0;
  else
    return (src->n - 1) * _BI_WORDSIZE + __fls (src->v[src->n-1]);
}
