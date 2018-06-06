/* uheapset.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

static int _um_walk_set (Heap_t h, const void *obj, size_t size, int flag,
                         int status, const char *fname, size_t lineno,
                         void *arg)
{
  if (flag == _FREEENTRY && status == _HEAPOK)
    {
      /* Cast away `const'. */
      memset ((void *)obj, (int)arg, size);
    }
  return status;
}


int _uheapset (Heap_t h, unsigned fill)
{
  struct _um_seg *seg;

  if (h == NULL)
    return _HEAPEMPTY;
  if (fill != 0)
    for (seg = h->seg_head; seg != NULL; seg = seg->next)
      seg->zero_limit = seg->end;
  return _uheap_walk2 (h, _um_walk_set, (void *)fill);
}
