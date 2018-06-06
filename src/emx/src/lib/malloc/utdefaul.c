/* utdefaul.c (emx+gcc) -- Copyright (c) 1996-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#include <InnoTekLIBC/thread.h>

Heap_t _utdefault (Heap_t h)
{
  _UM_MT_DECL
  Heap_t old;

  /* The same comment as in _udefault() applies here. */

  old = _UM_DEFAULT_TILED_HEAP;
  if (old == NULL)
    old = _um_tiled_heap;       /* ...which might be NULL */

  if (h != NULL)
    {
      if (h->magic != _UM_MAGIC_HEAP)
        return NULL;
      _UM_DEFAULT_TILED_HEAP = h;
    }
  return old;
}
