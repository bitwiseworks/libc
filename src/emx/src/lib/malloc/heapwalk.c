/* heapwalk.c (emx+gcc) -- Copyright (c) 1996-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <malloc.h>
#include <umalloc.h>
#include <emx/umalloc.h>
#include <InnoTekLIBC/thread.h>

int _heap_walk (_um_callback1 *callback)
{
  _UM_MT_DECL
  Heap_t    heap_reg = _UM_DEFAULT_REGULAR_HEAP;
  Heap_t    heap_tiled = _UM_DEFAULT_TILED_HEAP;
  int       rc;

  /* Initialize the heap pointers, in case _heap_walk() is called by a
     new thread before malloc(). */

  if (heap_reg == NULL)
    heap_reg = _um_init_default_regular_heap ();
  if (heap_tiled == NULL)
    heap_tiled = _um_init_default_tiled_heap ();

  /* First walk through the regular heap. */

  rc = _uheap_walk (heap_reg, callback);
  if (rc != 0)
    return rc;

  /* If there's no tiled heap or if it's identical to the regular
     heap, return the regular heap's status. */

  if (heap_reg == heap_tiled || heap_tiled == NULL)
    return rc;

  /* Walk through the tiled heap. */

  return _uheap_walk (heap_tiled, callback);
}
