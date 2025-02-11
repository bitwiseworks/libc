/* uheapmin.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

int _uheapmin (Heap_t h)
{
  struct _um_seg *seg, *next, **patch;
  struct _um_lump *lump;
  char *mem_end;
  size_t shrink_size, new_size, asize, fsize, min_size;
  char remove;

  if (h->magic != _UM_MAGIC_HEAP)
    return -1;
  if (h->release_fun == NULL && h->shrink_fun == NULL)
    return 0;

  _um_heap_lock (h);

  /* Examine all segments, releasing segments which don't contain any
     used lumps, shrinking segments which have a free lump at the end.
     Exactly those segments which contain one free lump don't contain
     any used lumps.  Examine the segments in LIFO manner to support
     sbrk(). */

  patch = &h->seg_head;
  for (seg = h->seg_head; seg != NULL; seg = next)
    {
      next = seg->next;
      remove = 0;
      lump = (struct _um_lump *)seg->start;
      if (seg != h->initial_seg
          && _UM_LUMP_STATUS (lump) == _UMS_FREE && _UM_LAST_LUMP (seg, lump))
        {
          /* This segment is unused and is not the initial segment.
             First try SHRINK_FUN because SHRINK_FUN returns status,
             so we can keep the segment if SHRINK_FUN fails.  For
             RELEASE_FUN, we have to assume successful deallocation of
             the segment. */

          if (h->shrink_fun != NULL)
            {
              _um_lump_unlink_heap (h, lump);
              shrink_size = 0;
              h->shrink_fun (h, seg->mem, seg->size, &shrink_size);
              if (shrink_size == 0)
                remove = 1;
              else if (shrink_size <= seg->size)
                {
                  if (_um_seg_setmem (h, seg, seg->mem, shrink_size,
                                      !_BLOCK_CLEAN) != h)
                    {
                      bzero (seg->mem, shrink_size);
                      remove = 1;
                    }
                }
              else
                {
                  _um_heap_maybe_unlock (h);
                  _um_abort ("_uheapmin: shrink_size=%x seg->size=%x\n", shrink_size, seg->size);
                }
            }
          else
            {
              _um_lump_unlink_heap (h, lump);
              h->release_fun (h, seg->mem, seg->size);
              remove = 1;
            }
        }

      if (remove)
        {
          *patch = next;
          h->n_segments -= 1;
        }
      else
        patch = &seg->next;
    }

  /* Shrink the most recently allocated segment, if possible.  Don't
     make the initial segment smaller than its initial size. */

  seg = h->seg_head;
  if (h->shrink_fun != NULL)
    {
      lump = _UM_PREV_LUMP ((struct _um_lump *)seg->end);
      if (_UM_LUMP_STATUS (lump) == _UMS_FREE)
        {
          min_size = seg == h->initial_seg ? h->initial_seg_size : 0;
          new_size = (char *)lump - (char *)seg->mem;
          if (new_size < min_size)
            new_size = min_size;

          /* Don't shrink the segment by less than one page. */

          if (new_size + _UM_PAGE_SIZE < seg->size)
            {
              _um_lump_unlink_heap (h, lump);
              shrink_size = new_size - min_size;
              h->shrink_fun (h, _UM_ADD (seg->mem, min_size),
                             seg->size - min_size, &shrink_size);
              _um_assert (shrink_size <= seg->size - min_size, h);
              _um_assert (shrink_size >= new_size - min_size, h);
              seg->size = shrink_size + min_size;

              /* Compute the number of bytes available at seg->start.
                 Drop the partial page at the end, if there is any. */

              mem_end = (char *)_UM_ADD (seg->mem, seg->size);
              asize = mem_end - (char *)seg->start;
              asize &= ~(_UM_PAGE_SIZE-1);
              _um_assert (asize > 0, h);

              /* Set the new segment end. */

              seg->end = _UM_ADD (seg->start, asize);
              seg->zero_limit = seg->end;

              /* If there's free space at the end (this happens only
                 if the SHRINK_FUN didn't deallocate all the excess
                 memory, or for the initial segment), make a new free
                 lump. */

              if ((void *)lump != seg->end)
                {
                  _um_assert ((void *)lump < seg->end, h);
                  fsize = (char *)seg->end - (char *)lump;
                  _um_assert (fsize % _UM_PAGE_SIZE == 0, h);
                  _um_lump_make_free (h, lump, seg, fsize);
                }
            }
        }
    }

  _um_heap_unlock (h);
  return 0;
}
