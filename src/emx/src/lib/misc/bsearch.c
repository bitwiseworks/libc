/* bsearch.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>

void *_STD(bsearch) (const void *key, const void *base, size_t num,
  size_t width, int (*compare)(const void *key, const void *element))
{
  int left, right, median, sign;
  const void *element;

  if (width <= 0)
    return NULL;
  left = 1; right = num;
  while (left <= right)
    {
      median = (left + right) / 2;
      element = (void *)((char *)base + (median-1)*width);
      sign = compare (key, element);
      if (sign == 0)
        return (void *)element;
      if (sign > 0)
        left = median + 1;
      else
        right = median - 1;
    }
  return NULL;
}
