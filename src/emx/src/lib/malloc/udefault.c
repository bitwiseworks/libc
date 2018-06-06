/* udefault.c (emx+gcc) -- Copyright (c) 1996-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#include <InnoTekLIBC/thread.h>

Heap_t _udefault (Heap_t h)
{
  _UM_MT_DECL
  Heap_t old;

  /* At program start and for new threads, _UM_DEFAULT_REGULAR_HEAP is
     NULL.  This means that _um_init_default_regular_heap() should be
     called to initialize _UM_DEFAULT_REGULAR_HEAP.  We want
     _udefault() to return a pointer to the heap, but we don't want
     _udefault() to create the heap as it might be replaced by the
     heap pointed to by H anyway.

     The following code makes _udefault() return the correct value for
     new threads if the heap already exists.  If the heap does not
     exist, NULL will be returned.

     Note that assigning _um_regular_heap to _UM_DEFAULT_REGULAR_HEAP
     (instead of OLD) would create a time window unless protected by
     the mutex semaphore used by _um_init_default_regular_heap()! */

  old = _UM_DEFAULT_REGULAR_HEAP;
  if (old == NULL)
    old = _um_regular_heap;     /* ...which might be NULL, see above */

  if (h != NULL)
    {
      if (h->magic != _UM_MAGIC_HEAP)
        return NULL;
      _UM_DEFAULT_REGULAR_HEAP = h;
    }
  return old;
}
