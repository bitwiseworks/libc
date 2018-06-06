/* utcalloc.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

void *_utcalloc (Heap_t h, size_t count, size_t size)
{
  size_t bytes;
  void *block;

  assert (h->magic == _UM_MAGIC_HEAP);
  if (h->magic != _UM_MAGIC_HEAP)
    return NULL;
  if (h->type & _HEAP_HIGHMEM)
    return NULL;

  bytes = count * size;
  if (size != 0 && bytes / size != count) /* Check for overflow */
    return NULL;

  _um_heap_lock (h);
  block = _um_alloc_no_lock (h, bytes, 4, _UMFI_ZERO | _UMFI_TILED);
  _um_heap_unlock (h);
  return block;
}
