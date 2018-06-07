/* udestroy.c (emx+gcc) -- Copyright (c) 1996-1997 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

int _udestroy (Heap_t h, int force)
{
  struct _um_seg *seg, *next;
  struct _um_lump *lump;
  size_t shrink_size;

  if (h->magic != _UM_MAGIC_HEAP
      || h == _um_regular_heap || h == _um_tiled_heap)
    return -1;

  _um_heap_lock (h);
  if (!force)
    {
      /* Check if all blocks have been freed.  As a crate is freed as
         soon as all its crumbs are freed, we have to check for lumps
         only.  If each segment contains just one free lump, no blocks
         are used. */

      for (seg = h->seg_head; seg != NULL; seg = seg->next)
        {
          lump = (struct _um_lump *)seg->start;
          if (_UM_LUMP_STATUS (lump) != _UMS_FREE
              || !_UM_LAST_LUMP (seg, lump))
            {
              /* At least one lump is used.  Return. */

              _um_heap_unlock (h);
              return 0;
            }
        }
    }
  
  h->magic = 0;
  _um_heap_unlock (h);

  /* Deallocate all the segments but the initial segment.  The
     segments are deallocated in LIFO manner to support sbrk(). */

  if (h->release_fun != NULL || h->shrink_fun != NULL)
    for (seg = h->seg_head; seg != NULL; seg = next)
      {
        next = seg->next;
        if (seg != h->initial_seg)
          {
            if (h->release_fun != NULL)
              h->release_fun (h, seg->mem, seg->size);
            else if (h->shrink_fun != NULL)
              {
                shrink_size = 0;
                h->shrink_fun (h, seg->mem, seg->size, &shrink_size);
              }
          }
      }
  h->seg_head = h->initial_seg;

  /* Deallocate memory contiguously added to the initial segment. */

  if (h->initial_seg->size != h->initial_seg_size)
    {
      seg = h->initial_seg;
      assert (seg->size > h->initial_seg_size);
      if (h->release_fun != NULL)
        h->release_fun (h, _UM_ADD (seg->mem, h->initial_seg_size),
                        seg->size - h->initial_seg_size);
      else if (h->shrink_fun != NULL)
        {
          shrink_size = 0;
          h->shrink_fun (h, _UM_ADD (seg->mem, h->initial_seg_size),
                         seg->size - h->initial_seg_size, &shrink_size);
        }
    }

  /* The semaphore is closed both by _uclose() and by _udestroy().
     This is required because _ucreate() sets the open count to 1 and
     n calls to _uopen() increment the open count to 1+n.  Then, n
     calls to _uclose() decrement the open count to 1 and _udestroy()
     decrements the open count to 0, destroying the semaphore. */

  _fmutex_close (&h->fsem);
  return 0;
}
