/* ialloc.c (emx+gcc) -- Copyright (c) 1996-1999 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

/* ALIGN must be a power of two. */

static void *_um_crumb_alloc (struct _um_crateset *crateset, size_t size,
                              size_t align, unsigned flags _UM_ASSERT_HEAP_PARAM(h))
{
  void *result;
  struct _um_crumb *crumb;
  struct _um_crate *crate;

  crumb = crateset->crumb_head;
  while (crumb != NULL)
    {
      crate = _PTR_FROM_UMINT (crumb->x.free.parent_crate, struct _um_crate);

      _um_assert (_UM_CRUMB_STATUS (crumb) == _UMS_FREE, h);
      _um_assert (crate->magic == _UM_MAGIC_CRATE, h);
      _um_assert (crate->parent_crateset == crateset, h);
      _um_assert (crate->used < crate->max, h);

      result = _UM_BLOCK_FROM_CRUMB (crumb);
      if (_UM_IS_ALIGNED (result, align)
          && (!(flags & _UMFI_TILED) || _UM_TILE_OK (result, size)))
        {
          _um_crumb_unlink (crateset, crumb);
          crate->used += 1;
          _UM_CRUMB_SET_STATUS (crumb, _UMS_USER);
          crumb->x.used.size = size;
          if (flags & _UMFI_ZERO)
            bzero (result, size);
          return result;
        }

      /* Move to the next crumb of the list. */

      crumb = crumb->x.free.next;
    }

  /* No (suitable) free crumb found in the free list.  Allocate from
     the most recently allocated crate. The most recently allocated
     crate is at the head of the list (ie, it's pointed to by
     crateset->crate_head).  The most recently allocated crate is the
     only one which has crate->init < crate->max. */

  crate = crateset->crate_head;
  if (crate == NULL || crate->init >= crate->max)
    return NULL;

  crumb = _UM_CRUMB_BY_INDEX (crate, crate->init);
  result = _UM_BLOCK_FROM_CRUMB (crumb);
  if (_UM_IS_ALIGNED (result, align)
      && (!(flags & _UMFI_TILED) || _UM_TILE_OK (result, size)))
    {
      _um_assert (_UM_IS_ALIGNED (crate, _UM_PARENT_ALIGN), h);
      crumb->x.used.parent_crate = _UMINT_FROM_PTR (crate) | _UMS_USER;
      crumb->x.used.size = size;
      crate->init += 1;
      crate->used += 1;
      if (flags & _UMFI_ZERO)    /* TODO: Not always required */
        bzero (result, size);
      return result;
    }

  /* Well, we could try the next crumb.  However, it's simpler to
     allocate a lump. */

  return NULL;
}


/* Attempt to allocate a lump without expanding the heap. */

static void *_um_lump_alloc_noexpand (Heap_t h, size_t size, size_t align,
                                      unsigned flags)
{
  int bucket_number;
  struct _um_lump *olump, *ulump, *slack;
  size_t rsize, osize, diff;
  void *block;
  struct _um_seg *seg;

  /* Set RSIZE to the rounded size of the lump. */

  rsize = _UM_ROUND_LUMP (size);
  if (rsize >= _UM_MAX_SIZE)
    return NULL;

  /* Find the smallest bucket for that lump size (`small' with respect
     to the size of the lumps, not to the number of the free lumps in
     that bucket).  There can't be a suitable free lump in any smaller
     bucket. */

  bucket_number = _um_find_bucket (rsize);

  /* Try all the lumps of the smallest bucket.  If this fails, try the
     next bigger bucket, and so on. */

  while (bucket_number < _UM_BUCKETS)
    {
      /* Examine all lumps of the current bucket. */

      for (olump = h->buckets[bucket_number].head;
           olump != NULL; olump = olump->x.free.next)
        {
          _um_assert (_UM_LUMP_STATUS (olump) == _UMS_FREE, h);

          /* Compute the rounded size of the current lump; try the
             next lump if it's too small. */

          osize = _UM_ROUND_LUMP (olump->size);
          if (osize < rsize)
            continue;

          /* Reject this lump if it's not big enough to obtain the
             requested alignment. */

          block = _UM_BLOCK_FROM_LUMP (olump);
          diff = _UM_ALIGN_DIFF (block, align);
          _um_assert (diff % _UM_PAGE_SIZE == 0, h);
          if (osize < diff + rsize)
            continue;

          /* Align the block as requested.  This may leave a free lump
             of DIFF bytes preceding the block. */

          block = _UM_ADD (block, diff);
          _um_assert (_UM_IS_ALIGNED (block, align), h);

          /* If tiling is requested, realign the block again if
             required.  Reject this lump if it's not big enough to
             obtain proper alignment for tiling or if its address is
             too high for tiling.  Update DIFF and BLOCK. */

          if (flags & _UMFI_TILED)
            {
              if (!_UM_TILE_TILED (block, size))
                continue;
              if (!_UM_TILE_ALIGNED (block, size))
                {
                  diff += _UM_ALIGN_DIFF (block, 0x10000);
                  if (osize < diff + rsize)
                    continue;
                  block = _UM_ADD (_UM_BLOCK_FROM_LUMP (olump), diff);
                  if (!_UM_TILE_TILED (block, size))
                    continue;
                }
              _um_assert (_UM_TILE_OK (block, size), h);
            }

          /* Remove the lump from the free list. */

          _um_lump_unlink_bucket (&h->buckets[bucket_number], olump);

          /* Initialize the lump.  ULUMP points to the aligned
             lump. */

          seg = _PTR_FROM_UMINT (olump->parent_seg, struct _um_seg);
          ulump = _UM_ADD (olump, diff);
          _um_assert (_UM_IS_ALIGNED (seg, _UM_PARENT_ALIGN), h);
          ulump->parent_seg = _UMINT_FROM_PTR (seg) | _UMS_USER;
          if (flags & _UMFI_CRATE)
            _UM_LUMP_SET_STATUS (ulump, _UMS_CRATE);
          _um_lump_set_size (ulump, size);

          /* If the lump is bigger than required, add the slack to the
             appropriate free list.  There's no need to coalesce with
             other free blocks as the slack is part of a (previously)
             free lump which cannot have neighboring fre lumps. */

          if (osize > diff + rsize)
            {
              _um_assert ((osize - rsize) % _UM_PAGE_SIZE == 0, h);
              slack = (struct _um_lump *)_UM_ADD (olump, diff + rsize);
              _um_lump_make_free (h, slack, seg, osize - (diff + rsize));
            }

          /* If padding was inserted before the block for alignment
             and/or tiling, add the padding to the free list.  There's
             no need to coalesce with other free blocks as the slack
             is part of a (previously) free lump which cannot have
             neighboring fre lumps. */

          if (diff > 0)
            {
              _um_assert (diff % _UM_PAGE_SIZE == 0, h);
              _um_lump_make_free (h, olump, seg, diff);
            }

          if (flags & _UMFI_ZERO) /* TODO: Not always required */
            bzero (block, size);

          return block;
        }
      ++bucket_number;
    }

  /* All suitable buckets searched, in vain.  Fail. */

  return NULL;
}


/* Call ALLOC_FUN to add SIZE bytes to the heap.  Return -1 on error,
   0 if ALLOC_FUN allocated less than SIZE bytes, or 1 if ALLOC_FUN
   allocated SIZE bytes or more. */

static int _um_heap_expand2 (Heap_t h, size_t size)
{
  void *memory;
  size_t mem_size, shrink_size;
  int clean;

  mem_size = size; clean = !_BLOCK_CLEAN;
  memory = h->alloc_fun (h, &mem_size, &clean);
  if (memory == NULL)
    return -1;

  if (_um_addmem_nolock (h, memory, mem_size, clean) != h)
    {
      /* _um_addmem_nolock() failed -- give back the memory. */

      if (h->release_fun != NULL)
        h->release_fun (h, memory, mem_size);
      else if (h->shrink_fun != NULL)
        {
          shrink_size = 0;
          h->shrink_fun (h, memory, mem_size, &shrink_size);
        }
      return -1;
    }

  return mem_size < size ? 0 : 1;
}


/* Attempt to expand the heap to enable allocation of a lump of SIZE
   bytes suitable for an ALIGN aligned allocation.
   Return true iff successful. */

static int _um_heap_expand (Heap_t h, size_t size, size_t align)
{
  size_t rsize, add_size;
  int r;

  /* Can't expand a fixed-size heap, ie, a heap which doesn't have an
     ALLOC_FUN. */

  if (h->alloc_fun == NULL)
    return 0;

  if (align < _UM_PAGE_SIZE / 2)
    rsize = _UM_ROUND_LUMP (size);
  else
    rsize = _UM_ROUND_LUMP (size + align + align / 2);
  if (rsize >= _UM_MAX_SIZE)
    return 0;
  add_size = rsize + _HEAP_MIN_SIZE;

  r = _um_heap_expand2 (h, add_size);
  if (r == 0)
    {
      /* ALLOC_FUN may allocate less memory than requested to avoid
         wasting the rest of a heap object.  Therefore we call
         ALLOC_FUN again if it did not allocate enough memory.  Note
         that we have to request the same amount of memory as in the
         first attempt!  Otherwise the ADD_SIZE bytes might be split
         in two heap objects. */

      r = _um_heap_expand2 (h, add_size);
    }
  return r > 0;
}


/* Allocate a lump, possibly expanding the heap to satisfy the
   allocation request. */

void *_um_lump_alloc (Heap_t h, size_t size, size_t align, unsigned flags)
{
  void *block;

  if (_UM_ROUND_LUMP (size) < size)
    return NULL;                /* Overflow */
  block = _um_lump_alloc_noexpand (h, size, align, flags);
  if (block != NULL)
    return block;
  if (!_um_heap_expand (h, size, align))
    return NULL;

  /* This should succeed -- note that the heap is locked.  Failure
     indicates an internal error or heap corruption. */

  return _um_lump_alloc_noexpand (h, size, align, flags);
}


static void *_um_crateset_alloc (Heap_t h, struct _um_crateset *crateset,
                                 size_t size, size_t align, unsigned flags)
{
  struct _um_crate *crate;
  size_t crate_size;
  void *block;
  int n;

  block = _um_crumb_alloc (crateset, size, align, flags _UM_ASSERT_HEAP_ARG(h));
  if (block != NULL)
    return block;

  /* Don't add a new crate if allocation failed due to alignment
     requirements. */

  if (crateset->crumb_head == NULL &&
      (crateset->crate_head == NULL
       || crateset->crate_head->init >= crateset->crate_head->max))
    {
      n = _UM_MAX_CRATE_SIZE / (crateset->crumb_size + _UM_CRUMB_OVERHEAD);
      crate_size = (sizeof (struct _um_crate)
                    + n * (crateset->crumb_size + _UM_CRUMB_OVERHEAD));
      crate = _um_lump_alloc (h, crate_size, _UM_PARENT_ALIGN, _UMFI_CRATE);
      if (crate != NULL)
        {
          crate->magic = _UM_MAGIC_CRATE;
          crate->parent_crateset = crateset;
          crate->parent_heap = h;
          crate->crumb_size = crateset->crumb_size;
          crate->max = n;
          crate->init = 0;
          crate->used = 0;
          crate->next = crateset->crate_head;
          crateset->crate_head = crate;

          /* Update the heap's number of crates and maximum number of
             crumbs. */

          h->n_crates += 1;
          h->max_crumbs += n;

          /* Try again to allocate a crumb. */

          return _um_crumb_alloc (crateset, size, align, flags _UM_ASSERT_HEAP_ARG(h));
        }
    }
  return NULL;
}


void *_um_alloc_no_lock (Heap_t h, size_t size, size_t align, unsigned flags)
{
  int i;
  void *block;

  if (h->n_cratesets > 0 && size <= h->cratesets[h->n_cratesets-1].crumb_size)
    {
      i = 0;
      while (size > h->cratesets[i].crumb_size)
        ++i;
      block = _um_crateset_alloc (h, &h->cratesets[i], size, align, flags);
      if (block != NULL)
        return block;
    }
  return _um_lump_alloc (h, size, align, flags);
}
