/* ucalloc.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

/* TODO: _BLOCK_CLEAN */

void *_ucalloc (Heap_t h, size_t count, size_t size)
{
  size_t bytes;
  void *block;
  unsigned flags;
  unsigned align;

  assert (h->magic == _UM_MAGIC_HEAP);
  if (h->magic != _UM_MAGIC_HEAP)
    return NULL;

  bytes = count * size;
  if (size != 0 && bytes / size != count) /* Check for overflow */
    return NULL;

  align = (size < 32 ? 4 : 16);

  _um_heap_lock (h);
  flags = _UMFI_ZERO;
  if (h->type & _HEAP_TILED)
    flags |= _UMFI_TILED;
  block = _um_alloc_no_lock (h, bytes, align, flags);
  _um_heap_unlock (h);
  return block;
}
