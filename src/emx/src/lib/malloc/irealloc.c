/* irealloc.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#include <errno.h>

static void *_um_crumb_expand (struct _um_crate *crate,
                               struct _um_crumb *crumb, size_t new_size)
{
  if (new_size > crate->crumb_size)
    return NULL;
  crumb->x.used.size = new_size;
  return _UM_BLOCK_FROM_CRUMB (crumb);
}

static void *_um_crumb_realloc (struct _um_crate *crate,
                                struct _um_crumb *crumb,
                                size_t new_size, size_t align, unsigned flags)
{
  void *r;
  Heap_t h;

  /* No locking required for _um_crumb_expand()! */
  r = _um_crumb_expand (crate, crumb, new_size);
  if (r != NULL || (flags & _UMFI_NOMOVE))
    return r;

  h = crate->parent_heap;
  _um_heap_lock (h);
  if (h->type & _HEAP_TILED)
    flags |= _UMFI_TILED;
  r = _um_alloc_no_lock (h, new_size, align, flags);
  if (r != NULL)
    {
      memcpy (r, _UM_BLOCK_FROM_CRUMB (crumb), crumb->x.used.size);
      _um_crumb_free_maybe_lock (crate, crumb, 0);
    }
  _um_heap_unlock (h);
  return r;
}

/* Try to expand the segment for the lump LUMP.  LUMP is either the
   last lump of the segment or is followed by a free lump which is the
   last lump of the segment. */

static int _um_lump_expand_seg (Heap_t h, struct _um_seg *seg,
                                struct _um_lump *lump, size_t new_size,
                                size_t align, unsigned flags)
{
  size_t rsize, nsize, expanded_size;
  int clean;

  /* We don't attempt to expand the segment if there's no EXPAND_FUN.
     This avoids calling ALLOC_FUN just to find out that it didn't
     allocate contiguous memory. */

  if (h->expand_fun == NULL)
    return 0;

  /* We try expanding the segment only for the segment which is at the
     head of the segment list, ie, for the most recently allocated
     segment.  This assumes that memory is allocated in a bottom-up
     fashion a la sbrk(). */

  if (seg != h->seg_head)
    return 0;

  /* Find out how much memory we need. */

  rsize = _UM_ROUND_LUMP (new_size);
  nsize = (char *)_UM_ADD (lump, rsize) - (char *)seg->mem;
  if (nsize <= seg->size)
    return 0;

  /* Attempt to expand the segment by calling EXPAND_FUN. */

  expanded_size = nsize; clean = !_BLOCK_CLEAN;
  if (!h->expand_fun (h, seg->mem, seg->size, &expanded_size, &clean)
      || expanded_size <= seg->size)
    return 0;

  /* Add the new memory to the segment. */

  if (_um_seg_addmem (h, seg, _UM_ADD (seg->mem, seg->size),
                      expanded_size - seg->size) != h)
    {
      /* We should shrink the segment here.  However, _seg_addmem()
         currently always returns h. */

      return 0;
    }

  /* Fail if the new size is too small. */

  if (expanded_size < nsize)
    return 0;

  return 1;
}


/* Try to expand a lump without expanding the segment.  LUMP must not
   be the last lump of the segment. */

static void *_um_lump_expand_up2 (Heap_t h, struct _um_seg *seg,
                                  struct _um_lump *lump, struct _um_lump *next,
                                  size_t new_size, size_t align,
                                  unsigned flags)
{
  struct _um_lump *slack;
  size_t rsize, osize;
  void *block;

  rsize = _UM_ROUND_LUMP (new_size);
  osize = _UM_ROUND_LUMP (lump->size) + _UM_ROUND_LUMP (next->size);
  if (osize < rsize)
    return NULL;

  /* TODO: attempt to move the block to satisfy alignment/tiling.
     Don't forget to update the computation of the new size in
     _um_lump_expand_seg()! */

  block = _UM_BLOCK_FROM_LUMP (lump);
  if (!_UM_IS_ALIGNED (block, align))
    return NULL;
  if ((flags & _UMFI_TILED) && !_UM_TILE_OK (block, new_size))
    return NULL;

  _um_lump_unlink_heap (seg->parent_heap, next);
  _um_lump_set_size (lump, new_size);

  /* If the new lump is bigger than required, add the slack to the
     appropriate free list.  As the lump wasn't free, it might have a
     neighboring free lump, so try to coalesce free lumps. */

  if (osize > rsize)
    {
      assert ((osize - rsize) % _UM_PAGE_SIZE == 0);
      slack = (struct _um_lump *)_UM_ADD (lump, rsize);
      _um_lump_coalesce_free (seg->parent_heap, slack, seg, osize - rsize);
    }
  return block;
}


static void *_um_lump_expand_up (Heap_t h, struct _um_seg *seg,
                                 struct _um_lump *lump, size_t new_size,
                                 size_t align, unsigned flags)
{
  struct _um_lump *next;
  void *block;

  if (!_UM_LAST_LUMP (seg, lump))
    {
      next = _UM_NEXT_LUMP (lump);
      if (_UM_LUMP_STATUS (next) != _UMS_FREE)
        return NULL;
      block =  _um_lump_expand_up2 (h, seg, lump, next, new_size,
                                    align, flags);
      if (block != NULL)
        return block;
      if (!_UM_LAST_LUMP (seg, next))
        return NULL;
    }

  /* Now, LUMP is either the last lump of the segment or followed by
     the last lump of the segment, which is a free lump. */

  /* TODO: Support realignment in _um_lump_expand_up2() and remove these
     checks. */

  block = _UM_BLOCK_FROM_LUMP (lump);
  if (!_UM_IS_ALIGNED (block, align))
    return NULL;
  if ((flags & _UMFI_TILED) && !_UM_TILE_OK (block, new_size))
    return NULL;

  /* Attempt to expand the segment. */

  if (!_um_lump_expand_seg (h, seg, lump, new_size, align, flags))
    return NULL;

  /* If _um_lump_expand_seg() succeeds, it has in fact enlarged the
     segment, either by adding a free lump or by extending the free
     lump following LUMP; the segment must be large enough now. */

  assert (!_UM_LAST_LUMP (seg, lump));
  next = _UM_NEXT_LUMP (lump);
  assert (_UM_LAST_LUMP (seg, next));
  assert (_UM_LUMP_STATUS (next) == _UMS_FREE);
  block = _um_lump_expand_up2 (h, seg, lump, next, new_size, align, flags);
  assert (block != NULL);
  return block;
}


static void *_um_lump_expand_down (struct _um_seg *seg,
                                   struct _um_lump *lump, size_t new_size,
                                   size_t align, unsigned flags)
{
  struct _um_lump *prev, *slack;
  size_t rsize, osize;
  void *block;

  if (_UM_FIRST_LUMP (seg, lump))
    return NULL;
  prev = _UM_PREV_LUMP (lump);
  if (_UM_LUMP_STATUS (prev) != _UMS_FREE)
    return NULL;
  rsize = _UM_ROUND_LUMP (new_size);
  osize = _UM_ROUND_LUMP (lump->size) + _UM_ROUND_LUMP (prev->size);
  if (osize < rsize)
    return NULL;

  /* TODO: attempt to move the block to satisfy alignment/tiling. */

  block = _UM_BLOCK_FROM_LUMP (prev);
  if (!_UM_IS_ALIGNED (block, align))
    return NULL;
  if ((flags & _UMFI_TILED) && !_UM_TILE_OK (block, new_size))
    return NULL;

  _um_lump_unlink_heap (seg->parent_heap, prev);
  prev->parent_seg = lump->parent_seg; /* Copy flag bits */
  memmove (block, _UM_BLOCK_FROM_LUMP (lump), lump->size);
  _um_lump_set_size (prev, new_size);

  /* If the new lump is bigger than required, add the slack to the
     appropriate free list.  As the lump wasn't free, it might have a
     neighboring free lump, so try to coalesce free lumps. */

  if (osize > rsize)
    {
      assert ((osize - rsize) % _UM_PAGE_SIZE == 0);
      slack = (struct _um_lump *)_UM_ADD (prev, rsize);
      _um_lump_coalesce_free (seg->parent_heap, slack, seg, osize - rsize);
    }
  return block;
}


static void *_um_lump_realloc (struct _um_seg *seg, struct _um_lump *lump,
                               size_t new_size, size_t align, unsigned flags)
{
  void *r;
  Heap_t h;
  size_t osize, rsize;
  struct _um_lump *slack;

  /* round the new size and make sure it doesn't accidentially wrap. */
  rsize = _UM_ROUND_LUMP (new_size);
  if (rsize < new_size)
    return NULL;

  h = seg->parent_heap;
  _um_heap_lock (h);
  if (h->type & _HEAP_TILED)
    flags |= _UMFI_TILED;
  osize = _UM_ROUND_LUMP (lump->size);
  r = _UM_BLOCK_FROM_LUMP (lump);
  if (rsize < osize)
    {
      /* TODO: Use crateset? */
      _um_lump_set_size (lump, new_size);
      assert ((osize - rsize) % _UM_PAGE_SIZE == 0);
      slack = (struct _um_lump *)_UM_ADD (lump, rsize);
      _um_lump_coalesce_free (h, slack, seg, osize - rsize);
    }
  else if (rsize > osize)
    {
      r = _um_lump_expand_up (h, seg, lump, new_size, align, flags);
      if (flags & _UMFI_NOMOVE)
        return r;
      if (r == NULL)
        r = _um_lump_expand_down (seg, lump, new_size, align, flags);
      if (r == NULL)
        {
          r = _um_lump_alloc (h, new_size, align, flags);
          if (r != NULL)
            {
              memcpy (r, _UM_BLOCK_FROM_LUMP (lump), lump->size);
              _um_lump_free_maybe_lock (seg, lump, 0);
            }
        }
    }
  else if (lump->size != new_size)
    _um_lump_set_size (lump, new_size);

  _um_heap_unlock (h);
  return r;
}


void *_um_realloc (void *block, size_t new_size, size_t align, unsigned flags)
{
  struct _um_hdr *hdr;
  _umagic *parent;
  void *ret;

  if (block == NULL)
    return NULL;
  if (new_size == 0)
    {
      _um_free_maybe_lock (block, 1);
      return NULL;
    }

  hdr = _HDR_FROM_BLOCK (block);
  if (_UM_HDR_STATUS (hdr) == _UMS_FREE)
    {
      _um_abort ("_um_free_maybe_lock: Tried to reallocate freed block - block=%p new_size=%x align=%x flags=%x\n",
                 block, new_size, align, flags);
      errno = EINVAL;
      return NULL;
    }
  parent = _PTR_FROM_UMINT (hdr->parent, _umagic);
  switch (*parent)
    {
    case _UM_MAGIC_CRATE:
      ret = _um_crumb_realloc ((struct _um_crate *)parent,
                               _UM_CRUMB_FROM_BLOCK (block), new_size,
                               align, flags);
      break;

    case _UM_MAGIC_SEG:
      ret = _um_lump_realloc ((struct _um_seg *)parent,
                              _UM_LUMP_FROM_BLOCK (block), new_size,
                              align, flags);
      break;

    default:
      {
        _um_abort ("_um_realloc: bad parent magic %x, parent=%p block=%p new_size=%x align=%x flags=%x\n",
                   *parent, parent, block, new_size, align, flags);
        errno = EINVAL;
        return NULL;
      }
    }
  if (ret)
    return ret;
  errno = ENOMEM;
  return NULL;
}
