/* ifree.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

static void _um_crate_free (struct _um_crateset *crateset,
                            struct _um_crate *crate)
{
  struct _um_crumb *crumb;
  struct _um_crate **patch;
  int i;

  /* Update the heap's number of crates and maximum number of
     crumbs. */

  crate->parent_heap->n_crates -= 1;
  crate->parent_heap->max_crumbs -= crate->max;

  /* Remove all the initialized crumbs of the crate from the free list. */

  for (i = 0; i < crate->init; ++i)
    {
      crumb = _UM_CRUMB_BY_INDEX (crate, i);
      assert (_UM_CRUMB_STATUS (crumb) == _UMS_FREE);
      assert (_PTR_FROM_UMINT (crumb->x.free.parent_crate, struct _um_crate) == crate);
      _um_crumb_unlink (crateset, crumb);
    }

  /* Remove the crate from the list of crates. */

  for (patch = &crateset->crate_head; *patch != NULL; patch = &(*patch)->next)
    if (*patch == crate)
      break;
  if (*patch == NULL)
    {
      _um_abort ("_um_crate_free: patch not found! patch=%p crate=%p crateset=%p\n",
                 patch, crate, crateset);
      return;
    }
  *patch = crate->next;

  /* Deallocate the crate. */

  _um_free_maybe_lock (crate, 0);
}


void _um_crumb_free_maybe_lock (struct _um_crate *crate,
                                struct _um_crumb *crumb, int lock)
{
  struct _um_crateset *crateset;

  crateset = crate->parent_crateset;
  if (lock)
    _um_heap_lock (crate->parent_heap);

  assert (_UM_CRUMB_STATUS (crumb) != _UMS_FREE);
  assert (crateset->crumb_size == crate->crumb_size);
  assert (crumb->x.used.size <= crate->crumb_size);
  assert (crate->used != 0);

  /* Insert the crumb at the head of the crateset's free list. */

  crumb->x.free.next = crateset->crumb_head;
  crumb->x.free.prev = NULL;
  if (crateset->crumb_head == NULL)
    crateset->crumb_tail = crumb;
  else
    crateset->crumb_head->x.free.prev = crumb;
  crateset->crumb_head = crumb;

  /* Mark the crumb as free. */

  _UM_CRUMB_SET_STATUS (crumb, _UMS_FREE);

  /* Set the crumb's size to the maximum value.  This is used by
     _uheapset(). */

  /* Adjust the crate. */

  crate->used -= 1;
  if (crate->used == 0)
    {
      /* The crate is no longer used.  Remove all its initialized
         crumbs from the free list and deallocate the crate. */

      _um_crate_free (crateset, crate);
    }

  if (lock)
    _um_heap_unlock (crate->parent_heap);
}


void _um_lump_free_maybe_lock (struct _um_seg *seg, struct _um_lump *lump,
                               int lock)
{
  Heap_t h;

  h = seg->parent_heap;
  if (lock)
    _um_heap_lock (h);

  assert (_UM_LUMP_STATUS (lump) != _UMS_FREE);
  _um_lump_coalesce_free (h, lump, seg, _UM_ROUND_LUMP (lump->size));

  if (lock)
    _um_heap_unlock (h);
}


void _um_free_maybe_lock (void *block, int lock)
{
  struct _um_hdr *hdr;
  _umagic *parent;

  if (block == NULL)
    return;

  hdr = _HDR_FROM_BLOCK (block);
  if (_UM_HDR_STATUS (hdr) == _UMS_FREE)
    {
      _um_abort ("_um_free_maybe_lock: Tried to free block twice - block=%p lock=%d\n",
                 block, lock);
      return;
    }
  parent = _PTR_FROM_UMINT (hdr->parent, _umagic);
  switch (*parent)
    {
    case _UM_MAGIC_CRATE:
      _um_crumb_free_maybe_lock ((struct _um_crate *)parent,
                                 _UM_CRUMB_FROM_BLOCK (block), lock);
      break;
    case _UM_MAGIC_SEG:
      _um_lump_free_maybe_lock ((struct _um_seg *)parent,
                                _UM_LUMP_FROM_BLOCK (block), lock);
      break;
    default:
      _um_abort ("_um_free_maybe_lock: bad parent magic %x, parent=%p block=%p lock=%d\n",
                 *parent, parent, block, lock);
      return;

    }
}
