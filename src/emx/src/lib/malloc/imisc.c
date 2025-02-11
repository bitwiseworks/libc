/* imisc.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>


static __inline__ void _um_lump_link_bucket (struct _um_bucket *bucket,
                                             struct _um_lump *lump)
{
  lump->x.free.prev = NULL;
  if (bucket->head != NULL)
    bucket->head->x.free.prev = lump;
  else
    bucket->tail = lump;
  lump->x.free.next = bucket->head;
  bucket->head = lump;
}


void _um_lump_unlink_bucket (struct _um_bucket *bucket, struct _um_lump *lump)
{
  if (lump->x.free.prev == NULL)
    bucket->head = lump->x.free.next;
  else
    lump->x.free.prev->x.free.next = lump->x.free.next;
  if (lump->x.free.next == NULL)
    bucket->tail = lump->x.free.prev;
  else
    lump->x.free.next->x.free.prev = lump->x.free.prev;
}


void _um_lump_unlink_heap (Heap_t h, struct _um_lump *lump)
{
  size_t rsize;
  int bucket;

  rsize = _UM_ROUND_LUMP (lump->size);
  bucket = _um_find_bucket (rsize);
  _um_lump_unlink_bucket (&h->buckets[bucket], lump);
}


void _um_lump_link_heap (Heap_t h, struct _um_lump *lump)
{
  size_t rsize;
  int bucket_number;

  rsize = _UM_ROUND_LUMP (lump->size);
  bucket_number = _um_find_bucket (rsize);
  _um_lump_link_bucket (&h->buckets[bucket_number], lump);
}


/* Turn uninitialized memory into a free lump.  RSIZE is the rounded
   total size, including `struct _um_lump' and the size word at the
   end.  This function does not coalesce the new lump with neighboring
   free lumps, therefore the segment need not be in a consistent
   state. */

void _um_lump_make_free (Heap_t h, struct _um_lump *lump,
                         struct _um_seg *seg, size_t rsize)
{
  int bucket_number;

  _um_assert (_UM_IS_ALIGNED (seg, _UM_PARENT_ALIGN), h);
  _um_assert (rsize % _UM_PAGE_SIZE == 0, h);
  lump->parent_seg = _UMINT_FROM_PTR (seg) | _UMS_FREE;
  _um_lump_set_size (lump, rsize - _UM_LUMP_OVERHEAD);

  bucket_number = _um_find_bucket (rsize);
  _um_lump_link_bucket (&h->buckets[bucket_number], lump);
}


/* Turn uninitialized memory into a free lump.  RSIZE is the rounded
   total size, including `struct _um_lump' and the size word at the
   end.  This function attempts to coalesce the new lump with
   neighboring free lumps.  Note that the segment must be in a
   consistent state (except for the free lump to be created). */

void _um_lump_coalesce_free (Heap_t h, struct _um_lump *lump,
                             struct _um_seg *seg, size_t rsize)
{
  _um_assert (_UM_IS_ALIGNED (seg, _UM_PARENT_ALIGN), h);
  _um_assert (rsize % _UM_PAGE_SIZE == 0, h);
  lump->parent_seg = _UMINT_FROM_PTR (seg) | _UMS_FREE;
  _um_lump_set_size (lump, rsize - _UM_LUMP_OVERHEAD);

  /* Coalesce with preceding free lump, if possible. */

  if (!_UM_FIRST_LUMP (seg, lump))
    {
      struct _um_lump *prev = _UM_PREV_LUMP (lump);
      if (_UM_LUMP_STATUS (prev) == _UMS_FREE)
        {
          _um_lump_unlink_heap (h, prev);
          _um_assert (_UM_IS_ALIGNED (seg, _UM_PARENT_ALIGN), h);
          prev->parent_seg = _UMINT_FROM_PTR (seg) | _UMS_FREE;

          /* It's important to round at least one of the sizes,
             otherwise the sum might become too short. */

          _um_lump_set_size (prev, (_UM_ROUND_LUMP (lump->size)
                                    + _UM_ROUND_LUMP (prev->size)
                                    - _UM_LUMP_OVERHEAD));
          lump = prev;
        }
    }

  /* Coalesce with succeeding free lump, if possible. */

  if (!_UM_LAST_LUMP (seg, lump))
    {
      struct _um_lump *next = _UM_NEXT_LUMP (lump);
      if (_UM_LUMP_STATUS (next) == _UMS_FREE)
        {
          _um_lump_unlink_heap (h, next);

          /* It's important to round at least one of the sizes,
             otherwise the sum might become too short. */

          _um_lump_set_size (lump, (_UM_ROUND_LUMP (lump->size)
                                    + _UM_ROUND_LUMP (next->size)
                                    - _UM_LUMP_OVERHEAD));
        }
    }

  _um_lump_link_heap (h, lump);
}

