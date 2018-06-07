/* ucreate2.c (emx+gcc) -- Copyright (c) 1996-1997 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

Heap_t _ucreate2 (void *memory, size_t size, int clean, unsigned type,
                  void *(*alloc_fun)(Heap_t, size_t *, int *),
                  void (*release_fun)(Heap_t, void *, size_t),
                  int (*expand_fun)(Heap_t, void *, size_t, size_t *, int *),
                  void (*shrink_fun)(Heap_t, void *, size_t, size_t *))
{
  struct _uheap *h;
  int i;
  unsigned long diff;
  unsigned t;

  /* Check the TYPE argument.  It must not contain any bits but
     _HEAP_REGULAR, _HEAP_TILED, _HEAP_SHARED, and _HEAP_HIGHMEM.
     Exactly one of _HEAP_REGULAR and _HEAP_TILED must be set.
     _HEAP_HIGHMEM and _HEAP_TILED are mutaly exclusive. */

  if (type & ~(_HEAP_REGULAR|_HEAP_TILED|_HEAP_SHARED|_HEAP_HIGHMEM))
    return NULL;
  t = type & (_HEAP_REGULAR|_HEAP_TILED);
  if (t != _HEAP_REGULAR && t != _HEAP_TILED)
      return NULL;
  if ((type & (_HEAP_HIGHMEM|_HEAP_TILED)) == (_HEAP_HIGHMEM|_HEAP_TILED))
      return NULL;


  /* Check the SIZE argument. */

  if (size < _HEAP_MIN_SIZE || size > _HEAP_MIN_SIZE + _UM_MAX_SIZE)
    return NULL;

  /* Compute DIFF, the number of bytes to insert at MEMORY to align H
     properly. */

  diff = _UM_ALIGN_DIFF (memory, 4);
  memory = _UM_ADD (memory, diff);
  size -= diff;
  if (size < _HEAP_MIN_SIZE)
    return NULL;

  /* Initialize the strcuture. */

  h = (struct _uheap *)memory;
  h->magic = 0;                 /* Will be set properly below */
  h->type = type;
  h->alloc_fun = alloc_fun;
  h->expand_fun = expand_fun;
  h->release_fun = release_fun;
  h->shrink_fun = shrink_fun;
  h->seg_head = NULL;
  h->n_segments = 0;
  h->n_crates = 0;
  h->max_crumbs = 0;
  for (i = 0; i < _UM_BUCKETS; ++i)
    {
      h->buckets[i].head = NULL;
      h->buckets[i].tail = NULL;
    }
  h->n_cratesets = 5;
  for (i = 0; i < h->n_cratesets; ++i)
    {
      h->cratesets[i].crate_head = NULL;
      h->cratesets[i].crumb_head = NULL;
      h->cratesets[i].crumb_tail = NULL;
    }
  h->cratesets[0].crumb_size = 4;
  h->cratesets[1].crumb_size = 8;
  h->cratesets[2].crumb_size = 12;
  h->cratesets[3].crumb_size = 16;
  h->cratesets[4].crumb_size = 20;

  /* Initialize the mutex semaphore. */
  if (_fmutex_create2 (&h->fsem, (type & _HEAP_SHARED) ? _FMC_SHARED : 0, "LIBC Heap") != 0)
    return NULL;

  /* The heap has been created, except for the initial segment. */

  h->magic = _UM_MAGIC_HEAP;

  /* Add the initial segment to the heap. */

  if (_um_addmem_nolock (h, _UM_ADD (h, sizeof (struct _uheap)),
                         size - sizeof (struct _uheap), clean) == NULL)
    {
      h->magic = 0;
      return NULL;
    }
  h->initial_seg = h->seg_head;
  h->initial_seg_size = h->initial_seg->size;
  return h;
}
