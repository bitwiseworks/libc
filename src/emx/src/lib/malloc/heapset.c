/* heapset.c (emx+gcc) -- Copyright (c) 1996-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#include <InnoTekLIBC/thread.h>

int _heapset (unsigned fill)
{
  _UM_MT_DECL
  int rc1, rc2;
  Heap_t    heap_reg = _UM_DEFAULT_REGULAR_HEAP;
  Heap_t    heap_tiled = _UM_DEFAULT_TILED_HEAP;

  /* Initialize the heap pointers, in case _heapset() is called by a
     new thread before malloc(). */

  if (_UM_DEFAULT_REGULAR_HEAP == NULL)
    heap_reg = _um_init_default_regular_heap ();
  if (_UM_DEFAULT_TILED_HEAP == NULL)
    heap_tiled = _um_init_default_tiled_heap ();

  /* First fill the regular heap. */

  rc1 = _uheapset (heap_reg, fill);
  if (rc1 != _HEAPOK && rc1 != _HEAPEMPTY)
    return rc1;

  /* If there's no tiled heap or if it's identical to the regular
     heap, return the regular heap's status. */

  if (heap_reg == heap_tiled || heap_tiled == NULL)
    return rc1;

  /* Fill the tiled heap.  Do not return _HEAPEMPTY if any of the two
     heaps is non-empty. */

  rc2 = _uheapset (heap_tiled, fill);
  if (rc2 == _HEAPEMPTY)
    return rc1;
  return rc2;
}
