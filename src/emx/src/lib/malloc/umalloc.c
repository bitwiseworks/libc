/* umalloc.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#include <errno.h>

void *_umalloc (Heap_t h, size_t size)
{
  void *block;
  unsigned flags;
  unsigned align = (size < 32 ? 4 : 16);

  assert (h->magic == _UM_MAGIC_HEAP);
  if (h->magic != _UM_MAGIC_HEAP)
    {
      errno = EINVAL;
      return NULL;
    }

  _um_heap_lock (h);
  flags = 0;
  if (h->type & _HEAP_TILED)
    flags |= _UMFI_TILED;
  block = _um_alloc_no_lock (h, size, align, flags);
  _um_heap_unlock (h);
  if (block)
      return block;
  errno = ENOMEM;
  return NULL;
}
