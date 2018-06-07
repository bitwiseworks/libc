/* uwalk2.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

int _uheap_walk2 (Heap_t h, _um_callback2 *callback, void *arg)
{
  int ret;

  if (h == NULL || h->magic != _UM_MAGIC_HEAP)
    return _HEAPBADBEGIN;
  _um_heap_lock (h);
  ret = _um_walk_no_lock (h, callback, arg);
  _um_heap_unlock (h);
  return ret;
}
