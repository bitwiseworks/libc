/* utmalloc.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

void *_utmalloc (Heap_t h, size_t size)
{
  void *block;

  assert (h->magic == _UM_MAGIC_HEAP);
  if (h->magic != _UM_MAGIC_HEAP)
    return NULL;
  if (h->type & _HEAP_HIGHMEM)
    return NULL;
  _um_heap_lock (h);
  block = _um_alloc_no_lock (h, size, 4, _UMFI_TILED);
  _um_heap_unlock (h);
  return block;
}
