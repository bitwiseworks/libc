/* iaddmem.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

Heap_t _um_seg_addmem (Heap_t h, struct _um_seg *seg, void *memory,
                       size_t size)
{
  struct _um_lump *lump;
  size_t asize, osize;

  assert (_UM_ADD (seg->mem, seg->size) == memory);

  /* Expand the segment SEG by adding free space at the end.  First,
     compute the new number of bytes available at START.  Drop the
     partial page at the end, if there is any. */

  asize = (char *)seg->mem + seg->size + size - (char *)seg->start;
  asize &= ~(_UM_PAGE_SIZE-1);

  /* Compute the old size of the segment's pages. */

  osize = (char *)seg->end - (char *)seg->start;

  /* Incorporate the new memory as free space into the heap if it adds
     at least one page. */

  if (asize > osize)
    {
      lump = _UM_PREV_LUMP ((struct _um_lump *)seg->end);
      if (_UM_LUMP_STATUS (lump) == _UMS_FREE)
        {
          /* Coalesce the last free lump of the segment with the new
             memory.  This is done here instead of relying on
             _um_lump_coalesce_free() because the segment is not in a
             consistent state. */

          _um_lump_unlink_heap (h, lump);
          _um_lump_set_size (lump, (_UM_ROUND_LUMP (lump->size) + asize - osize
                                    - _UM_LUMP_OVERHEAD));
          _um_lump_link_heap (h, lump);
        }
      else
        {
          /* The last lump of the segment is used.  Add a free lump at
             the end of the segment.  Don't attempt to coalesce
             because the segment is not in a consistent state. */

          _um_lump_make_free (h, (struct _um_lump *)_UM_ADD (seg->start,
                                                             osize),
                              seg, asize - osize);
        }

      /* Adjust the segment control structure. */

      seg->end = _UM_ADD (seg->start, asize);
      /* TODO: zero_limit */
    }

  /* Update SIZE anyway.  This avoids calling _um_seg_addmem() over
     and over from _um_lump_expand_seg(). */

  seg->size += size;
  return h;
}


Heap_t _um_seg_setmem (Heap_t h, struct _um_seg *seg, void *memory,
                       size_t size, int clean)
{
  char *start, *mem_end;
  unsigned long diff;
  size_t asize;

  seg->size = size;
  seg->mem = memory;

  /* Align START+_UM_LUMP_HEADER_SIZE on a _UM_PAGE_SIZE boundary. */

  start = _UM_ADD (seg, sizeof (struct _um_seg));
  diff = _UM_ALIGN_DIFF (start + _UM_LUMP_HEADER_SIZE, _UM_PAGE_SIZE);
  start += diff;
  assert (_UM_IS_ALIGNED (start + _UM_LUMP_HEADER_SIZE, _UM_PAGE_SIZE));

  /* Compute the number of bytes available at START.  Drop the partial
     page at the end, if there is any. */

  mem_end = (char *)_UM_ADD (seg->mem, seg->size);
  if (start > mem_end)
    return NULL;
  asize = mem_end - start;
  asize &= ~(_UM_PAGE_SIZE-1);
  if (asize == 0)
    return NULL;

  seg->start = (void *)start;
  seg->end = (void *)(start + asize);
  seg->zero_limit = clean ? seg->start : seg->end;

  _um_lump_make_free (h, (struct _um_lump *)start, seg, asize);
  return h;
}


Heap_t _um_addmem_nolock (Heap_t h, void *memory, size_t size, int clean)
{
  struct _um_seg *seg;
  unsigned long diff;

  /* First check if an existing segment is to be expanded
     contiguously.  This is very important in order to avoid splitting
     the heap into a lot of segments if sbrk() is used by
     alloc_fun(). */

  for (seg = h->seg_head; seg != NULL; seg = seg->next)
    if (_UM_ADD (seg->mem, seg->size) == memory)
      break;

  if (seg != NULL)
    return _um_seg_addmem (h, seg, memory, size);

  /* _UM_PARENT_ALIGN is the minimum alignment.  Add one to be able to
     allocate at least a one-byte block.  Add another page for
     alignment (well, mis-alignment) of the pages. */

  if (size < (sizeof (struct _um_seg) + _UM_ROUND_LUMP (_UM_PARENT_ALIGN)
              + _UM_PAGE_SIZE))
    return NULL;

  /* Align SEG to make the low bits of the address available for
     _UMF_MASK. */

  diff = _UM_ALIGN_DIFF (memory, _UM_PARENT_ALIGN);
  seg = (struct _um_seg *)_UM_ADD (memory, diff);
  assert (((unsigned long)seg & _UMF_MASK) == 0);

  /* Create a segment. */

  if (_um_seg_setmem (h, seg, memory, size, clean) != h)
    return NULL;

  /* Add the segment to the heap. */

  seg->parent_heap = h;
  seg->next = h->seg_head;
  h->seg_head = seg;
  h->n_segments += 1;

  seg->magic = _UM_MAGIC_SEG;
  return h;
}
